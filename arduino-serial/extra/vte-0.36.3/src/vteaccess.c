/*
 * Copyright (C) 2002,2003 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * SECTION: vte-access
 * @short_description: Accessibility peer of #VteTerminal
 *
 * The accessibility peer of a #VteTerminal, implementing GNOME's accessibility
 * framework.
 */

#include <config.h>

#include <atk/atk.h>
#include <gtk/gtk.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif
#include <string.h>
#include "debug.h"
#include "vte.h"
#include "vteaccess.h"
#include "vteint.h"

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#include <glib/gi18n-lib.h>

enum {
        ACTION_MENU,
        LAST_ACTION
};

#define VTE_TERMINAL_ACCESSIBLE_PRIVATE_DATA "VteTerminalAccessiblePrivateData"

typedef struct _VteTerminalAccessiblePrivate {
	gboolean snapshot_contents_invalid;	/* This data is stale. */
	gboolean snapshot_caret_invalid;	/* This data is stale. */
	GString *snapshot_text;		/* Pointer to UTF-8 text. */
	GArray *snapshot_characters;	/* Offsets to character begin points. */
	GArray *snapshot_attributes;	/* Attributes, per byte. */
	GArray *snapshot_linebreaks;	/* Offsets to line breaks. */
	gint snapshot_caret;       /* Location of the cursor (in characters). */

	char *action_descriptions[LAST_ACTION];
} VteTerminalAccessiblePrivate;

enum direction {
	direction_previous = -1,
	direction_current = 0,
	direction_next = 1
};

static gunichar vte_terminal_accessible_get_character_at_offset(AtkText *text,
								gint offset);
static gpointer vte_terminal_accessible_parent_class;

G_DEFINE_TYPE(VteTerminalAccessibleFactory, vte_terminal_accessible_factory, ATK_TYPE_OBJECT_FACTORY)

static const char *vte_terminal_accessible_action_names[] = {
        "menu",
        NULL
};

static const char *vte_terminal_accessible_action_descriptions[] = {
        "Popup context menu",
        NULL
};

/* Create snapshot private data. */
static VteTerminalAccessiblePrivate *
vte_terminal_accessible_new_private_data(void)
{
	VteTerminalAccessiblePrivate *priv;
	priv = g_slice_new0(VteTerminalAccessiblePrivate);
	priv->snapshot_text = NULL;
	priv->snapshot_characters = NULL;
	priv->snapshot_attributes = NULL;
	priv->snapshot_linebreaks = NULL;
	priv->snapshot_caret = -1;
	priv->snapshot_contents_invalid = TRUE;
	priv->snapshot_caret_invalid = TRUE;
	return priv;
}

/* Free snapshot private data. */
static void
vte_terminal_accessible_free_private_data(VteTerminalAccessiblePrivate *priv)
{
	gint i;

	g_assert(priv != NULL);
	if (priv->snapshot_text != NULL) {
		g_string_free(priv->snapshot_text, TRUE);
	}
	if (priv->snapshot_characters != NULL) {
		g_array_free(priv->snapshot_characters, TRUE);
	}
	if (priv->snapshot_attributes != NULL) {
		g_array_free(priv->snapshot_attributes, TRUE);
	}
	if (priv->snapshot_linebreaks != NULL) {
		g_array_free(priv->snapshot_linebreaks, TRUE);
	}
	for (i = 0; i < LAST_ACTION; i++) {
		g_free (priv->action_descriptions[i]);
	}
	g_slice_free(VteTerminalAccessiblePrivate, priv);
}

static gint
offset_from_xy (VteTerminalAccessiblePrivate *priv,
		gint x, gint y)
{
	gint offset;
	gint linebreak;
	gint next_linebreak;

	if (y >= (gint) priv->snapshot_linebreaks->len)
		y = priv->snapshot_linebreaks->len -1;

	linebreak = g_array_index (priv->snapshot_linebreaks, int, y);
	if (y + 1 == (gint) priv->snapshot_linebreaks->len)
		next_linebreak = priv->snapshot_characters->len;
	else
		next_linebreak = g_array_index (priv->snapshot_linebreaks, int, y + 1);

	offset = linebreak + x;
	if (offset >= next_linebreak)
		offset = next_linebreak -1;
	return offset;
}

static void
xy_from_offset (VteTerminalAccessiblePrivate *priv,
		guint offset, gint *x, gint *y)
{
	guint i, linebreak;
	gint cur_x, cur_y;
	gint cur_offset = 0;

	cur_x = -1;
	cur_y = -1;
	for (i = 0; i < priv->snapshot_linebreaks->len; i++) {
		linebreak = g_array_index (priv->snapshot_linebreaks, int, i);
		if (offset < linebreak) {
			cur_x = offset - cur_offset;
			cur_y = i - 1;
			break;

		}  else {
			cur_offset = linebreak;
		}
	}
	if (i == priv->snapshot_linebreaks->len) {
		if (offset <= priv->snapshot_characters->len) {
			cur_x = offset - cur_offset;
			cur_y = i - 1;
		}
	}
	*x = cur_x;
	*y = cur_y;
}

/* "Oh yeah, that's selected.  Sure." callback. */
static gboolean
all_selected(VteTerminal *terminal, glong column, glong row, gpointer data)
{
	return TRUE;
}

static void
emit_text_caret_moved(GObject *object, glong caret)
{
	_vte_debug_print(VTE_DEBUG_SIGNALS|VTE_DEBUG_ALLY,
			"Accessibility peer emitting "
			"`text-caret-moved'.\n");
	g_signal_emit_by_name(object, "text-caret-moved", caret);
}

static void
emit_text_changed_insert(GObject *object,
			 const char *text, glong offset, glong len)
{
	glong start, count;
	if (len == 0) {
		return;
	}
	/* Convert the byte offsets to character offsets. */
	start = g_utf8_pointer_to_offset (text, text + offset);
	count = g_utf8_pointer_to_offset (text + offset, text + offset + len);
	_vte_debug_print(VTE_DEBUG_SIGNALS|VTE_DEBUG_ALLY,
			"Accessibility peer emitting "
			"`text-changed::insert' (%ld, %ld) (%ld, %ld).\n"
			"Inserted text was `%.*s'.\n",
			offset, len, start, count,
			(int) len, text + offset);
	g_signal_emit_by_name(object, "text-changed::insert", start, count);
}

static void
emit_text_changed_delete(GObject *object,
			 const char *text, glong offset, glong len)
{
	glong start, count;
	if (len == 0) {
		return;
	}
	/* Convert the byte offsets to characters. */
	start = g_utf8_pointer_to_offset (text, text + offset);
	count = g_utf8_pointer_to_offset (text + offset, text + offset + len);
	_vte_debug_print(VTE_DEBUG_SIGNALS|VTE_DEBUG_ALLY,
			"Accessibility peer emitting "
			"`text-changed::delete' (%ld, %ld) (%ld, %ld).\n"
			"Deleted text was `%.*s'.\n",
			offset, len, start, count,
			(int) len, text + offset);
	g_signal_emit_by_name(object, "text-changed::delete", start, count);
}

static void
vte_terminal_accessible_update_private_data_if_needed(AtkObject *text,
						      char **old, glong *olen)
{
	VteTerminal *terminal;
	VteTerminalAccessiblePrivate *priv;
	struct _VteCharAttributes attrs;
	char *next, *tmp;
	long row, offset, caret;
	long ccol, crow;
	guint i;

	g_assert(VTE_IS_TERMINAL_ACCESSIBLE(text));

	/* Retrieve the private data structure.  It must already exist. */
	priv = g_object_get_data(G_OBJECT(text),
				 VTE_TERMINAL_ACCESSIBLE_PRIVATE_DATA);
	g_assert(priv != NULL);

	/* If nothing's changed, just return immediately. */
	if ((priv->snapshot_contents_invalid == FALSE) &&
	    (priv->snapshot_caret_invalid == FALSE)) {
		if (old) {
			if (priv->snapshot_text) {
				*old = g_malloc(priv->snapshot_text->len + 1);
				memcpy(*old,
				       priv->snapshot_text->str,
				       priv->snapshot_text->len);
				(*old)[priv->snapshot_text->len] = '\0';
				if (olen) {
					*olen = priv->snapshot_text->len;
				}
			} else {
				*old = g_strdup("");
				if (olen) {
					*olen = 0;
				}
			}
		} else {
			if (olen) {
				g_assert_not_reached();
			}
		}
		return;
	}

	/* Re-read the contents of the widget if the contents have changed. */
	terminal = VTE_TERMINAL(gtk_accessible_get_widget(GTK_ACCESSIBLE(text)));
	if (priv->snapshot_contents_invalid) {
		/* Free the outdated snapshot data, unless the caller
		 * wants it. */
		if (old) {
			if (priv->snapshot_text != NULL) {
				*old = priv->snapshot_text->str;
				if (olen) {
					*olen = priv->snapshot_text->len;
				}
				g_string_free(priv->snapshot_text, FALSE);
			} else {
				*old = g_strdup("");
				if (olen) {
					*olen = 0;
				}
			}
		} else {
			if (olen) {
				g_assert_not_reached();
			}
			if (priv->snapshot_text != NULL) {
				g_string_free(priv->snapshot_text, TRUE);
			}
		}
		priv->snapshot_text = NULL;

		/* Free the character offsets and allocate a new array to hold
		 * them. */
		if (priv->snapshot_characters != NULL) {
			g_array_free(priv->snapshot_characters, TRUE);
		}
		priv->snapshot_characters = g_array_new(FALSE, FALSE, sizeof(int));

		/* Free the attribute lists and allocate a new array to hold
		 * them. */
		if (priv->snapshot_attributes != NULL) {
			g_array_free(priv->snapshot_attributes, TRUE);
		}
		priv->snapshot_attributes = g_array_new(FALSE, FALSE,
							sizeof(struct _VteCharAttributes));

		/* Free the linebreak offsets and allocate a new array to hold
		 * them. */
		if (priv->snapshot_linebreaks != NULL) {
			g_array_free(priv->snapshot_linebreaks, TRUE);
		}
		priv->snapshot_linebreaks = g_array_new(FALSE, FALSE, sizeof(int));

		/* Get a new view of the uber-label. */
		tmp = vte_terminal_get_text_include_trailing_spaces(terminal,
								    all_selected,
								    NULL,
								    priv->snapshot_attributes);
		if (tmp == NULL) {
			/* Aaargh!  We're screwed. */
			return;
		}
		priv->snapshot_text = g_string_new_len(tmp,
						       priv->snapshot_attributes->len);
		g_free(tmp);

		/* Get the offsets to the beginnings of each character. */
		i = 0;
		next = priv->snapshot_text->str;
		while (i < priv->snapshot_attributes->len) {
			g_array_append_val(priv->snapshot_characters, i);
			next = g_utf8_next_char(next);
			if (next == NULL) {
				break;
			} else {
				i = next - priv->snapshot_text->str;
			}
		}
		/* Find offsets for the beginning of lines. */
		for (i = 0, row = 0; i < priv->snapshot_characters->len; i++) {
			/* Get the attributes for the current cell. */
			offset = g_array_index(priv->snapshot_characters,
					       int, i);
			attrs = g_array_index(priv->snapshot_attributes,
					      struct _VteCharAttributes,
					      offset);
			/* If this character is on a row different from the row
			 * the character we looked at previously was on, then
			 * it's a new line and we need to keep track of where
			 * it is. */
			if ((i == 0) || (attrs.row != row)) {
				_vte_debug_print(VTE_DEBUG_ALLY,
						"Row %d/%ld begins at %u.\n",
						priv->snapshot_linebreaks->len,
						attrs.row, i);
				g_array_append_val(priv->snapshot_linebreaks, i);
			}
			row = attrs.row;
		}
		/* Add the final line break. */
		g_array_append_val(priv->snapshot_linebreaks, i);
		/* We're finished updating this. */
		priv->snapshot_contents_invalid = FALSE;
	}

	/* Update the caret position. */
	vte_terminal_get_cursor_position(terminal, &ccol, &crow);
	_vte_debug_print(VTE_DEBUG_ALLY,
			"Cursor at (%ld, " "%ld).\n", ccol, crow);

	/* Get the offsets to the beginnings of each line. */
	caret = -1;
	for (i = 0; i < priv->snapshot_characters->len; i++) {
		/* Get the attributes for the current cell. */
		offset = g_array_index(priv->snapshot_characters,
				       int, i);
		attrs = g_array_index(priv->snapshot_attributes,
				      struct _VteCharAttributes,
				      offset);
		/* If this cell is "before" the cursor, move the
		 * caret to be "here". */
		if ((attrs.row < crow) ||
		    ((attrs.row == crow) && (attrs.column < ccol))) {
			caret = i + 1;
		}
	}

	/* If no cells are before the caret, then the caret must be
	 * at the end of the buffer. */
	if (caret == -1) {
		caret = priv->snapshot_characters->len;
	}

	/* Notify observers if the caret moved. */
	if (caret != priv->snapshot_caret) {
		priv->snapshot_caret = caret;
		emit_text_caret_moved(G_OBJECT(text), caret);
	}

	/* Done updating the caret position, whether we needed to or not. */
	priv->snapshot_caret_invalid = FALSE;

	_vte_debug_print(VTE_DEBUG_ALLY,
			"Refreshed accessibility snapshot, "
			"%ld cells, %ld characters.\n",
			(long)priv->snapshot_attributes->len,
			(long)priv->snapshot_characters->len);
}

/* A signal handler to catch "text-inserted/deleted/modified" signals. */
static void
vte_terminal_accessible_text_modified(VteTerminal *terminal, gpointer data)
{
	VteTerminalAccessiblePrivate *priv;
	char *old, *current;
	glong offset, caret_offset, olen, clen;
	gint old_snapshot_caret;

	g_assert(VTE_IS_TERMINAL_ACCESSIBLE(data));

	priv = g_object_get_data(G_OBJECT(data),
				 VTE_TERMINAL_ACCESSIBLE_PRIVATE_DATA);
	g_assert(priv != NULL);

	old_snapshot_caret = priv->snapshot_caret;
	priv->snapshot_contents_invalid = TRUE;
	vte_terminal_accessible_update_private_data_if_needed(ATK_OBJECT(data),
							      &old, &olen);
	g_assert(old != NULL);

	current = priv->snapshot_text->str;
	clen = priv->snapshot_text->len;

	if ((guint) priv->snapshot_caret < priv->snapshot_characters->len) {
		caret_offset = g_array_index(priv->snapshot_characters,
				int, priv->snapshot_caret);
	} else {
		/* caret was not in the line */
		caret_offset = clen;
	}

	/* Find the offset where they don't match. */
	offset = 0;
	while ((offset < olen) && (offset < clen)) {
		if (old[offset] != current[offset]) {
			break;
		}
		offset++;
	}

        /* Check if we just backspaced over a space. */
	if ((olen == offset) &&
		       	(caret_offset < olen && old[caret_offset] == ' ') &&
			(old_snapshot_caret == priv->snapshot_caret + 1)) {
                priv->snapshot_text->str = old;
		priv->snapshot_text->len = caret_offset + 1;
		emit_text_changed_delete(G_OBJECT(data),
					 old, caret_offset, 1);
		priv->snapshot_text->str = current;
		priv->snapshot_text->len = clen;
	}


	/* At least one of them had better have more data, right? */
	if ((offset < olen) || (offset < clen)) {
		/* Back up from both end points until we find the *last* point
		 * where they differed. */
		gchar *op = old + olen;
		gchar *cp = current + clen;
		while (op > old + offset && cp > current + offset) {
			gchar *opp = g_utf8_prev_char (op);
			gchar *cpp = g_utf8_prev_char (cp);
			if (g_utf8_get_char (opp) != g_utf8_get_char (cpp)) {
				break;
			}
			op = opp;
			cp = cpp;
		}
		/* recompute the respective lengths */
		olen = op - old;
		clen = cp - current;
		/* At least one of them has to have text the other
		 * doesn't. */
		g_assert((clen > offset) || (olen > offset));
		g_assert((clen >= 0) && (olen >= 0));
		/* Now emit a deleted signal for text that was in the old
		 * string but isn't in the new one... */
		if (olen > offset) {
			gchar *saved_str = priv->snapshot_text->str;
			gsize saved_len = priv->snapshot_text->len;

			priv->snapshot_text->str = old;
			priv->snapshot_text->len = olen;
			emit_text_changed_delete(G_OBJECT(data),
						 old,
						 offset,
						 olen - offset);
			priv->snapshot_text->str = saved_str;
			priv->snapshot_text->len = saved_len;
		}
		/* .. and an inserted signal for text that wasn't in the old
		 * string but is in the new one. */
		if (clen > offset) {
			emit_text_changed_insert(G_OBJECT(data),
						 current,
						 offset,
						 clen - offset);
		}
	}

	g_free(old);
}

/* A signal handler to catch "text-scrolled" signals. */
static void
vte_terminal_accessible_text_scrolled(VteTerminal *terminal,
				      gint howmuch,
				      gpointer data)
{
	VteTerminalAccessiblePrivate *priv;
	struct _VteCharAttributes attr;
	long delta;
	guint i, len;

	g_assert(VTE_IS_TERMINAL_ACCESSIBLE(data));
	g_assert(howmuch != 0);

	priv = g_object_get_data(G_OBJECT(data),
				 VTE_TERMINAL_ACCESSIBLE_PRIVATE_DATA);
	g_assert(priv != NULL);

	if (((howmuch < 0) && (howmuch <= -terminal->row_count)) ||
	    ((howmuch > 0) && (howmuch >= terminal->row_count))) {
		/* All of the text was removed. */
		if (priv->snapshot_text != NULL) {
			if (priv->snapshot_text->str != NULL) {
				emit_text_changed_delete(G_OBJECT(data),
							 priv->snapshot_text->str,
							 0,
							 priv->snapshot_text->len);
			}
		}
		priv->snapshot_contents_invalid = TRUE;
		vte_terminal_accessible_update_private_data_if_needed(ATK_OBJECT(data),
								      NULL,
								      NULL);
		/* All of the present text was added. */
		if (priv->snapshot_text != NULL) {
			if (priv->snapshot_text->str != NULL) {
				emit_text_changed_insert(G_OBJECT(data),
							 priv->snapshot_text->str,
							 0,
							 priv->snapshot_text->len);
			}
		}
		return;
	}
	/* Find the start point. */
	delta = 0;
	if (priv->snapshot_attributes != NULL) {
		if (priv->snapshot_attributes->len > 0) {
			attr = g_array_index(priv->snapshot_attributes,
					     struct _VteCharAttributes,
					     0);
			delta = attr.row;
		}
	}
	/* We scrolled up, so text was added at the top and removed
	 * from the bottom. */
	if ((howmuch < 0) && (howmuch > -terminal->row_count)) {
		gboolean inserted = FALSE;
		howmuch = -howmuch;
		if (priv->snapshot_attributes != NULL &&
				priv->snapshot_text != NULL) {
			/* Find the first byte that scrolled off. */
			for (i = 0; i < priv->snapshot_attributes->len; i++) {
				attr = g_array_index(priv->snapshot_attributes,
						struct _VteCharAttributes,
						i);
				if (attr.row >= delta + terminal->row_count - howmuch) {
					break;
				}
			}
			if (i < priv->snapshot_attributes->len) {
				/* The rest of the string was deleted -- make a note. */
				emit_text_changed_delete(G_OBJECT(data),
						priv->snapshot_text->str,
						i,
						priv->snapshot_attributes->len - i);
			}
			inserted = TRUE;
		}
		/* Refresh.  Note that i is now the length of the data which
		 * we expect to have left over. */
		priv->snapshot_contents_invalid = TRUE;
		vte_terminal_accessible_update_private_data_if_needed(ATK_OBJECT(data),
								      NULL,
								      NULL);
		/* If we now have more text than before, the initial portion
		 * was added. */
		if (inserted) {
			len = priv->snapshot_text->len;
			if (len > i) {
				emit_text_changed_insert(G_OBJECT(data),
							 priv->snapshot_text->str,
							 0,
							 len - i);
			}
		}
		return;
	}
	/* We scrolled down, so text was added at the bottom and removed
	 * from the top. */
	if ((howmuch > 0) && (howmuch < terminal->row_count)) {
		gboolean inserted = FALSE;
		if (priv->snapshot_attributes != NULL &&
				priv->snapshot_text != NULL) {
			/* Find the first byte that wasn't scrolled off the top. */
			for (i = 0; i < priv->snapshot_attributes->len; i++) {
				attr = g_array_index(priv->snapshot_attributes,
						struct _VteCharAttributes,
						i);
				if (attr.row >= delta + howmuch) {
					break;
				}
			}
			/* That many bytes disappeared -- make a note. */
			emit_text_changed_delete(G_OBJECT(data),
					priv->snapshot_text->str,
					0,
					i);
			/* Figure out how much text was left, and refresh. */
			i = strlen(priv->snapshot_text->str + i);
			inserted = TRUE;
		}
		priv->snapshot_contents_invalid = TRUE;
		vte_terminal_accessible_update_private_data_if_needed(ATK_OBJECT(data),
								      NULL,
								      NULL);
		/* Any newly-added string data is new, so note that it was
		 * inserted. */
		if (inserted) {
			len = priv->snapshot_text->len;
			if (len > i) {
				emit_text_changed_insert(G_OBJECT(data),
							 priv->snapshot_text->str,
							 i,
							 len - i);
			}
		}
		return;
	}
	g_assert_not_reached();
}

/* A signal handler to catch "cursor-moved" signals. */
static void
vte_terminal_accessible_invalidate_cursor(VteTerminal *terminal, gpointer data)
{
	VteTerminalAccessiblePrivate *priv;

	g_assert(VTE_IS_TERMINAL_ACCESSIBLE(data));

	priv = g_object_get_data(G_OBJECT(data),
				 VTE_TERMINAL_ACCESSIBLE_PRIVATE_DATA);
	g_assert(priv != NULL);

	_vte_debug_print(VTE_DEBUG_ALLY,
			"Invalidating accessibility cursor.\n");
	priv->snapshot_caret_invalid = TRUE;
	vte_terminal_accessible_update_private_data_if_needed(ATK_OBJECT(data),
							      NULL, NULL);
}

/* Handle title changes by resetting the description. */
static void
vte_terminal_accessible_title_changed(VteTerminal *terminal, gpointer data)
{
	g_assert(VTE_IS_TERMINAL_ACCESSIBLE(data));
	g_assert(VTE_IS_TERMINAL(terminal));
	atk_object_set_description(ATK_OBJECT(data), terminal->window_title);
}

/* Reflect focus-in events. */
static gboolean
vte_terminal_accessible_focus_in(VteTerminal *terminal, GdkEventFocus *event,
				 gpointer data)
{
	g_assert(VTE_IS_TERMINAL_ACCESSIBLE(data));
	g_assert(VTE_IS_TERMINAL(terminal));
	g_signal_emit_by_name(data, "focus-event", TRUE);
	atk_object_notify_state_change(ATK_OBJECT(data),
				       ATK_STATE_FOCUSED, TRUE);

	return FALSE;
}

/* Reflect focus-out events. */
static gboolean
vte_terminal_accessible_focus_out(VteTerminal *terminal, GdkEventFocus *event,
				  gpointer data)
{
	g_assert(VTE_IS_TERMINAL_ACCESSIBLE(data));
	g_assert(VTE_IS_TERMINAL(terminal));
	g_signal_emit_by_name(data, "focus-event", FALSE);
	atk_object_notify_state_change(ATK_OBJECT(data),
				       ATK_STATE_FOCUSED, FALSE);

	return FALSE;
}

/* Reflect visibility-notify events. */
static gboolean
vte_terminal_accessible_visibility_notify(VteTerminal *terminal,
					  GdkEventVisibility *event,
					  gpointer data)
{
	GtkWidget *widget;
	gboolean visible;
	g_assert(VTE_IS_TERMINAL_ACCESSIBLE(data));
	g_assert(VTE_IS_TERMINAL(terminal));
	visible = event->state != GDK_VISIBILITY_FULLY_OBSCURED;
	/* The VISIBLE state indicates that this widget is "visible". */
	atk_object_notify_state_change(ATK_OBJECT(data),
				       ATK_STATE_VISIBLE,
				       visible);
	widget = GTK_WIDGET(terminal);
	while (visible) {
		if (gtk_widget_get_toplevel(widget) == widget) {
			break;
		}
		if (widget == NULL) {
			break;
		}
		visible = visible && (gtk_widget_get_visible(widget));
		widget = gtk_widget_get_parent(widget);
	}
	/* The SHOWING state indicates that this widget, and all of its
	 * parents up to the toplevel, are "visible". */
	atk_object_notify_state_change(ATK_OBJECT(data),
				       ATK_STATE_SHOWING,
				       visible);

	return FALSE;
}

static void
vte_terminal_accessible_selection_changed (VteTerminal *terminal,
					   gpointer data)
{
	g_assert(VTE_IS_TERMINAL_ACCESSIBLE(data));
	g_assert(VTE_IS_TERMINAL(terminal));

	g_signal_emit_by_name (data, "text_selection_changed");
}

static void
vte_terminal_accessible_destroyed (GtkWidget     *widget,
                                 GtkAccessible *accessible)
{
  gtk_accessible_set_widget (accessible, NULL);
  atk_object_notify_state_change (ATK_OBJECT (accessible), ATK_STATE_DEFUNCT, TRUE);
}

static gboolean
focus_cb (GtkWidget     *widget,
	        GdkEventFocus *event)
{
	AtkObject* accessible;

	accessible = gtk_widget_get_accessible (widget);

	atk_object_notify_state_change (accessible, ATK_STATE_FOCUSED, event->in);
	return FALSE;
}

static void
notify_cb (GObject    *obj,
	         GParamSpec *pspec)
{
	GtkWidget* widget = GTK_WIDGET (obj);
	AtkObject* atk_obj = gtk_widget_get_accessible (widget);
	AtkState state;
	gboolean value;

	if (strcmp (pspec->name, "has-focus") == 0)
		/*
		 * We use focus-in-event and focus-out-event signals to catch
		 * focus changes so we ignore this.
	 */
		return;
	else if (strcmp (pspec->name, "visible") == 0) {
		state = ATK_STATE_VISIBLE;
		value = gtk_widget_get_visible (widget);
	  } else if (strcmp (pspec->name, "sensitive") == 0) {
		state = ATK_STATE_SENSITIVE;
		value = gtk_widget_get_sensitive (widget);
	  } else
		return;

	atk_object_notify_state_change (atk_obj, state, value);
	if (state == ATK_STATE_SENSITIVE)
		atk_object_notify_state_change (atk_obj, ATK_STATE_ENABLED, value);
}

/* Translate GtkWidget::size-allocate to AtkComponent::bounds-changed */
static void
size_allocate_cb (GtkWidget     *widget,
	                GtkAllocation *allocation)
{
	AtkObject* accessible;
	AtkRectangle rect;

	accessible = gtk_widget_get_accessible (widget);
	rect.x = allocation->x;
	rect.y = allocation->y;
	rect.width = allocation->width;
	rect.height = allocation->height;
	g_signal_emit_by_name (accessible, "bounds_changed", &rect);
}

/* Translate GtkWidget mapped state into AtkObject showing */
static gint
map_cb (GtkWidget *widget)
{
	AtkObject *accessible;

	accessible = gtk_widget_get_accessible (widget);
	atk_object_notify_state_change (accessible, ATK_STATE_SHOWING,
	                                gtk_widget_get_mapped (widget));
	return 1;
}

static void
vte_terminal_initialize (AtkObject *obj, gpointer data)
{
	VteTerminal *terminal;
	AtkObject *parent;

	ATK_OBJECT_CLASS (vte_terminal_accessible_parent_class)->initialize (obj, data);

	gtk_accessible_set_widget (GTK_ACCESSIBLE (obj), GTK_WIDGET (data));

	terminal = VTE_TERMINAL (data);

	_vte_terminal_accessible_ref(terminal);

	g_object_set_data(G_OBJECT(obj),
			  VTE_TERMINAL_ACCESSIBLE_PRIVATE_DATA,
			  vte_terminal_accessible_new_private_data());

	g_signal_connect(terminal, "text-inserted",
			 G_CALLBACK(vte_terminal_accessible_text_modified),
			 obj);
	g_signal_connect(terminal, "text-deleted",
			 G_CALLBACK(vte_terminal_accessible_text_modified),
			 obj);
	g_signal_connect(terminal, "text-modified",
			 G_CALLBACK(vte_terminal_accessible_text_modified),
			 obj);
	g_signal_connect(terminal, "text-scrolled",
			 G_CALLBACK(vte_terminal_accessible_text_scrolled),
			 obj);
	g_signal_connect(terminal, "cursor-moved",
			 G_CALLBACK(vte_terminal_accessible_invalidate_cursor),
			 obj);
	g_signal_connect(terminal, "window-title-changed",
			 G_CALLBACK(vte_terminal_accessible_title_changed),
			 obj);

	/* everything below copied from gtkwidgetaccessible.c */
	g_signal_connect(terminal, "focus-in-event",
			 G_CALLBACK(vte_terminal_accessible_focus_in),
			 obj);
	g_signal_connect(terminal, "focus-out-event",
		G_CALLBACK(vte_terminal_accessible_focus_out), obj);
	g_signal_connect(terminal, "visibility-notify-event",
		G_CALLBACK(vte_terminal_accessible_visibility_notify), obj);
	g_signal_connect(terminal, "selection-changed",
		G_CALLBACK(vte_terminal_accessible_selection_changed), obj);

	if (GTK_IS_WIDGET(gtk_widget_get_parent(GTK_WIDGET(terminal)))) {
		parent = gtk_widget_get_accessible(gtk_widget_get_parent ((GTK_WIDGET(terminal))));
		if (ATK_IS_OBJECT(parent)) {
			atk_object_set_parent(obj, parent);
		}
	}

	atk_object_set_name(obj, "Terminal");
	atk_object_set_description(obj,
				   terminal->window_title ?
				   terminal->window_title :
				   "");

	atk_object_notify_state_change(obj,
				       ATK_STATE_FOCUSABLE, TRUE);
	atk_object_notify_state_change(obj,
				       ATK_STATE_EXPANDABLE, FALSE);
	atk_object_notify_state_change(obj,
				       ATK_STATE_RESIZABLE, TRUE);
	obj->role = ATK_ROLE_TERMINAL;

	g_signal_connect_after (terminal, "destroy",
				G_CALLBACK (vte_terminal_accessible_destroyed), obj);
	g_signal_connect_after (terminal, "focus-in-event", G_CALLBACK (focus_cb), NULL);
	g_signal_connect_after (terminal, "focus-out-event", G_CALLBACK (focus_cb), NULL);
	g_signal_connect (terminal, "notify", G_CALLBACK (notify_cb), NULL);
	g_signal_connect (terminal, "size-allocate",
		G_CALLBACK (size_allocate_cb), NULL);
  g_signal_connect (terminal, "map", G_CALLBACK (map_cb), NULL);
  g_signal_connect (terminal, "unmap", G_CALLBACK (map_cb), NULL);
}

/**
 * vte_terminal_accessible_new:
 * @terminal: a #VteTerminal
 *
 * Creates a new accessibility peer for the terminal widget.
 *
 * Returns: the new #AtkObject
 */
AtkObject *
vte_terminal_accessible_new(VteTerminal *terminal)
{
	AtkObject *accessible;
	GObject *object;

	g_return_val_if_fail(VTE_IS_TERMINAL(terminal), NULL);

	object = g_object_new(VTE_TYPE_TERMINAL_ACCESSIBLE, NULL);
	accessible = ATK_OBJECT (object);
	atk_object_initialize(accessible, terminal);

	return accessible;
}

static void
vte_terminal_accessible_finalize(GObject *object)
{
	VteTerminalAccessiblePrivate *priv;
	GtkAccessible *accessible = NULL;
        GtkWidget *widget;

	_vte_debug_print(VTE_DEBUG_ALLY, "Finalizing accessible peer.\n");

	g_assert(VTE_IS_TERMINAL_ACCESSIBLE(object));
	accessible = GTK_ACCESSIBLE(object);
        widget = gtk_accessible_get_widget (accessible);

	if (widget != NULL) {
		g_object_remove_weak_pointer(G_OBJECT(widget),
					     (gpointer*)(gpointer)&widget);
		g_signal_handlers_disconnect_matched(widget,
						     G_SIGNAL_MATCH_FUNC |
						     G_SIGNAL_MATCH_DATA,
						     0, 0, NULL,
						     vte_terminal_accessible_text_modified,
						     object);
		g_signal_handlers_disconnect_matched(widget,
						     G_SIGNAL_MATCH_FUNC |
						     G_SIGNAL_MATCH_DATA,
						     0, 0, NULL,
						     vte_terminal_accessible_text_scrolled,
						     object);
		g_signal_handlers_disconnect_matched(widget,
						     G_SIGNAL_MATCH_FUNC |
						     G_SIGNAL_MATCH_DATA,
						     0, 0, NULL,
						     vte_terminal_accessible_invalidate_cursor,
						     object);
		g_signal_handlers_disconnect_matched(widget,
						     G_SIGNAL_MATCH_FUNC |
						     G_SIGNAL_MATCH_DATA,
						     0, 0, NULL,
						     vte_terminal_accessible_title_changed,
						     object);
		g_signal_handlers_disconnect_matched(widget,
						     G_SIGNAL_MATCH_FUNC |
						     G_SIGNAL_MATCH_DATA,
						     0, 0, NULL,
						     vte_terminal_accessible_focus_in,
						     object);
		g_signal_handlers_disconnect_matched(widget,
						     G_SIGNAL_MATCH_FUNC |
						     G_SIGNAL_MATCH_DATA,
						     0, 0, NULL,
						     vte_terminal_accessible_focus_out,
						     object);
		g_signal_handlers_disconnect_matched(widget,
						     G_SIGNAL_MATCH_FUNC |
						     G_SIGNAL_MATCH_DATA,
						     0, 0, NULL,
						     vte_terminal_accessible_visibility_notify,
						     object);
	}
	priv = g_object_get_data(object,
				 VTE_TERMINAL_ACCESSIBLE_PRIVATE_DATA);
	if (priv != NULL) {
		vte_terminal_accessible_free_private_data(priv);
		g_object_set_data(object,
				  VTE_TERMINAL_ACCESSIBLE_PRIVATE_DATA,
				  NULL);
	}
	G_OBJECT_CLASS(vte_terminal_accessible_parent_class)->finalize(object);
}

static gchar *
vte_terminal_accessible_get_text(AtkText *text,
				 gint start_offset, gint end_offset)
{
	VteTerminalAccessiblePrivate *priv;
	int start, end;
	gchar *ret;

        /* Swap around if start is greater than end */
        if (start_offset > end_offset && end_offset != -1) {
                gint tmp;

                tmp = start_offset;
                start_offset = end_offset;
                end_offset = tmp;
        }

	g_assert((start_offset >= 0) && (end_offset >= -1));

	vte_terminal_accessible_update_private_data_if_needed(ATK_OBJECT(text),
							      NULL, NULL);

	priv = g_object_get_data(G_OBJECT(text),
				 VTE_TERMINAL_ACCESSIBLE_PRIVATE_DATA);
	_vte_debug_print(VTE_DEBUG_ALLY,
			"Getting text from %d to %d of %d.\n",
			start_offset, end_offset,
			priv->snapshot_characters->len);
	g_assert(ATK_IS_TEXT(text));

	/* If the requested area is after all of the text, just return an
	 * empty string. */
	if (start_offset >= (int) priv->snapshot_characters->len) {
		return g_strdup("");
	}

	/* Map the offsets to, er, offsets. */
	start = g_array_index(priv->snapshot_characters, int, start_offset);
	if ((end_offset == -1) || (end_offset >= (int) priv->snapshot_characters->len) ) {
		/* Get everything up to the end of the buffer. */
		end = priv->snapshot_text->len;
	} else {
		/* Map the stopping point. */
		end = g_array_index(priv->snapshot_characters, int, end_offset);
	}
	if (end <= start) {
		ret = g_strdup ("");
	} else {
		ret = g_malloc(end - start + 1);
		memcpy(ret, priv->snapshot_text->str + start, end - start);
		ret[end - start] = '\0';
	}
	return ret;
}

/* Map a subsection of the text with before/at/after char/word/line specs
 * into a run of Unicode characters.  (The interface is specifying characters,
 * not bytes, plus that saves us from having to deal with parts of multibyte
 * characters, which are icky.) */
static gchar *
vte_terminal_accessible_get_text_somewhere(AtkText *text,
					   gint offset,
					   AtkTextBoundary boundary_type,
					   enum direction direction,
					   gint *start_offset,
					   gint *end_offset)
{
	VteTerminalAccessiblePrivate *priv;
	VteTerminal *terminal;
	gunichar current, prev, next;
	guint start, end, line;

	vte_terminal_accessible_update_private_data_if_needed(ATK_OBJECT(text),
							      NULL, NULL);

	priv = g_object_get_data(G_OBJECT(text),
				 VTE_TERMINAL_ACCESSIBLE_PRIVATE_DATA);
	terminal = VTE_TERMINAL(gtk_accessible_get_widget (GTK_ACCESSIBLE(text)));

	_vte_debug_print(VTE_DEBUG_ALLY,
			"Getting %s %s at %d of %d.\n",
			(direction == direction_current) ? "this" :
			((direction == direction_next) ? "next" : "previous"),
			(boundary_type == ATK_TEXT_BOUNDARY_CHAR) ? "char" :
			((boundary_type == ATK_TEXT_BOUNDARY_LINE_START) ? "line (start)" :
			((boundary_type == ATK_TEXT_BOUNDARY_LINE_END) ? "line (end)" :
			((boundary_type == ATK_TEXT_BOUNDARY_WORD_START) ? "word (start)" :
			((boundary_type == ATK_TEXT_BOUNDARY_WORD_END) ? "word (end)" :
			((boundary_type == ATK_TEXT_BOUNDARY_SENTENCE_START) ? "sentence (start)" :
			((boundary_type == ATK_TEXT_BOUNDARY_SENTENCE_END) ? "sentence (end)" : "unknown")))))),
			offset, priv->snapshot_attributes->len);
	g_assert(priv->snapshot_text != NULL);
	g_assert(priv->snapshot_characters != NULL);
	if (offset >= (int) priv->snapshot_characters->len) {
		return g_strdup("");
	}
	g_assert(offset < (int) priv->snapshot_characters->len);
	g_assert(offset >= 0);

	switch (boundary_type) {
		case ATK_TEXT_BOUNDARY_CHAR:
			/* We're either looking at the character at this
			 * position, the one before it, or the one after it. */
			offset += direction;
			start = MAX(offset, 0);
			end = MIN(offset + 1, (int) priv->snapshot_attributes->len);
			break;
		case ATK_TEXT_BOUNDARY_WORD_START:
			/* Back up to the previous non-word-word transition. */
			while (offset > 0) {
				prev = vte_terminal_accessible_get_character_at_offset(text, offset - 1);
				if (vte_terminal_is_word_char(terminal, prev)) {
					offset--;
				} else {
					break;
				}
			}
			start = offset;
			/* If we started in a word and we're looking for the
			 * word before this one, keep searching by backing up
			 * to the previous non-word character and then searching
			 * for the word-start before that. */
			if (direction == direction_previous) {
				while (offset > 0) {
					prev = vte_terminal_accessible_get_character_at_offset(text, offset - 1);
					if (!vte_terminal_is_word_char(terminal, prev)) {
						offset--;
					} else {
						break;
					}
				}
				while (offset > 0) {
					prev = vte_terminal_accessible_get_character_at_offset(text, offset - 1);
					if (vte_terminal_is_word_char(terminal, prev)) {
						offset--;
					} else {
						break;
					}
				}
				start = offset;
			}
			/* If we're looking for the word after this one,
			 * search forward by scanning forward for the next
			 * non-word character, then the next word character
			 * after that. */
			if (direction == direction_next) {
				while (offset < (int) priv->snapshot_characters->len) {
					next = vte_terminal_accessible_get_character_at_offset(text, offset);
					if (vte_terminal_is_word_char(terminal, next)) {
						offset++;
					} else {
						break;
					}
				}
				while (offset < (int) priv->snapshot_characters->len) {
					next = vte_terminal_accessible_get_character_at_offset(text, offset);
					if (!vte_terminal_is_word_char(terminal, next)) {
						offset++;
					} else {
						break;
					}
				}
				start = offset;
			}
			/* Now find the end of this word. */
			while (offset < (int) priv->snapshot_characters->len) {
				current = vte_terminal_accessible_get_character_at_offset(text, offset);
				if (vte_terminal_is_word_char(terminal, current)) {
					offset++;
				} else {
					break;
				}

			}
			/* Now find the next non-word-word transition */
			while (offset < (int) priv->snapshot_characters->len) {
				next = vte_terminal_accessible_get_character_at_offset(text, offset);
				if (!vte_terminal_is_word_char(terminal, next)) {
					offset++;
				} else {
					break;
				}
			}
			end = offset;
			break;
		case ATK_TEXT_BOUNDARY_WORD_END:
			/* Back up to the previous word-non-word transition. */
			current = vte_terminal_accessible_get_character_at_offset(text, offset);
			while (offset > 0) {
				prev = vte_terminal_accessible_get_character_at_offset(text, offset - 1);
				if (vte_terminal_is_word_char(terminal, prev) &&
				    !vte_terminal_is_word_char(terminal, current)) {
					break;
				} else {
					offset--;
					current = prev;
				}
			}
			start = offset;
			/* If we're looking for the word end before this one, 
			 * keep searching by backing up to the previous word 
			 * character and then searching for the word-end 
			 * before that. */
			if (direction == direction_previous) {
				while (offset > 0) {
					prev = vte_terminal_accessible_get_character_at_offset(text, offset - 1);
					if (vte_terminal_is_word_char(terminal, prev)) {
						offset--;
					} else {
						break;
					}
				}
				current = vte_terminal_accessible_get_character_at_offset(text, offset);
				while (offset > 0) {
					prev = vte_terminal_accessible_get_character_at_offset(text, offset - 1);
					if (vte_terminal_is_word_char(terminal, prev) &&
					    !vte_terminal_is_word_char(terminal, current)) {
						break;
					} else {
						offset--;
						current = prev;
					}
				}
				start = offset;
			}
			/* If we're looking for the word end after this one,
			 * search forward by scanning forward for the next
			 * word character, then the next non-word character
			 * after that. */
			if (direction == direction_next) {
				while (offset < (int) priv->snapshot_characters->len) {
					current = vte_terminal_accessible_get_character_at_offset(text, offset);
					if (!vte_terminal_is_word_char(terminal, current)) {
						offset++;
					} else {
						break;
					}
				}
				while (offset < (int) priv->snapshot_characters->len) {
					current = vte_terminal_accessible_get_character_at_offset(text, offset);
					if (vte_terminal_is_word_char(terminal, current)) {
						offset++;
					} else {
						break;
					}
				}
				start = offset;
			}
			/* Now find the next word end. */
			while (offset < (int) priv->snapshot_characters->len) {
				current = vte_terminal_accessible_get_character_at_offset(text, offset);
				if (!vte_terminal_is_word_char(terminal, current)) {
					offset++;
				} else {
					break;
				}
			}
			while (offset < (int) priv->snapshot_characters->len) {
				current = vte_terminal_accessible_get_character_at_offset(text, offset);
				if (vte_terminal_is_word_char(terminal, current)) {
					offset++;
				} else {
					break;
				}
			}
			end = offset;
			break;
		case ATK_TEXT_BOUNDARY_LINE_START:
		case ATK_TEXT_BOUNDARY_LINE_END:
			/* Figure out which line we're on.  If the start of the
			 * i'th line is before the offset, then i could be the
			 * line we're looking for. */
			line = 0;
			for (line = 0;
			     line < priv->snapshot_linebreaks->len;
			     line++) {
				if (g_array_index(priv->snapshot_linebreaks,
						  int, line) > offset) {
					line--;
					break;
				}
			}
			_vte_debug_print(VTE_DEBUG_ALLY,
					"Character %d is on line %d.\n",
					offset, line);
			/* Perturb the line number to handle before/at/after. */
			line += direction;
			line = MIN(line, priv->snapshot_linebreaks->len - 1);
			/* Read the offsets for this line. */
			start = g_array_index(priv->snapshot_linebreaks,
						      int, line);
			line++;
			line = MIN(line, priv->snapshot_linebreaks->len - 1);
			end = g_array_index(priv->snapshot_linebreaks,
						    int, line);
			_vte_debug_print(VTE_DEBUG_ALLY,
					"Line runs from %d to %d.\n",
					start, end);
			break;
		case ATK_TEXT_BOUNDARY_SENTENCE_START:
		case ATK_TEXT_BOUNDARY_SENTENCE_END:
			/* This doesn't make sense.  Fall through. */
		default:
			start = end = 0;
			break;
	}
	*start_offset = start = MIN(start, priv->snapshot_characters->len - 1);
	*end_offset = end = CLAMP(end, start, priv->snapshot_characters->len);
	return vte_terminal_accessible_get_text(text, start, end);
}

static gchar *
vte_terminal_accessible_get_text_before_offset(AtkText *text, gint offset,
					       AtkTextBoundary boundary_type,
					       gint *start_offset,
					       gint *end_offset)
{
	g_assert(VTE_IS_TERMINAL_ACCESSIBLE(text));
	vte_terminal_accessible_update_private_data_if_needed(ATK_OBJECT(text),
							      NULL, NULL);
	return vte_terminal_accessible_get_text_somewhere(text,
							  offset,
							  boundary_type,
							  -1,
							  start_offset,
							  end_offset);
}

static gchar *
vte_terminal_accessible_get_text_after_offset(AtkText *text, gint offset,
					      AtkTextBoundary boundary_type,
					      gint *start_offset,
					      gint *end_offset)
{
	g_assert(VTE_IS_TERMINAL_ACCESSIBLE(text));
	vte_terminal_accessible_update_private_data_if_needed(ATK_OBJECT(text),
							      NULL, NULL);
	return vte_terminal_accessible_get_text_somewhere(text,
							  offset,
							  boundary_type,
							  1,
							  start_offset,
							  end_offset);
}

static gchar *
vte_terminal_accessible_get_text_at_offset(AtkText *text, gint offset,
					   AtkTextBoundary boundary_type,
					   gint *start_offset,
					   gint *end_offset)
{
	g_assert(VTE_IS_TERMINAL_ACCESSIBLE(text));
	vte_terminal_accessible_update_private_data_if_needed(ATK_OBJECT(text),
							      NULL, NULL);
	return vte_terminal_accessible_get_text_somewhere(text,
							  offset,
							  boundary_type,
							  0,
							  start_offset,
							  end_offset);
}

static gunichar
vte_terminal_accessible_get_character_at_offset(AtkText *text, gint offset)
{
	VteTerminalAccessiblePrivate *priv;
	char *unichar;
	gunichar ret;

	vte_terminal_accessible_update_private_data_if_needed(ATK_OBJECT(text),
							      NULL, NULL);

	priv = g_object_get_data(G_OBJECT(text),
				 VTE_TERMINAL_ACCESSIBLE_PRIVATE_DATA);

	g_assert(offset < (int) priv->snapshot_characters->len);

	unichar = vte_terminal_accessible_get_text(text, offset, offset + 1);
	ret = g_utf8_get_char(unichar);
	g_free(unichar);

	return ret;
}

static gint
vte_terminal_accessible_get_caret_offset(AtkText *text)
{
	VteTerminalAccessiblePrivate *priv;

	vte_terminal_accessible_update_private_data_if_needed(ATK_OBJECT(text),
							      NULL, NULL);

	priv = g_object_get_data(G_OBJECT(text),
				 VTE_TERMINAL_ACCESSIBLE_PRIVATE_DATA);

	return priv->snapshot_caret;
}

static AtkAttributeSet *
get_attribute_set (struct _VteCharAttributes attr)
{
	AtkAttributeSet *set = NULL;
	AtkAttribute *at;

	if (attr.underline) {
		at = g_new (AtkAttribute, 1);
		at->name = g_strdup ("underline");
		at->value = g_strdup ("true");
		set = g_slist_append (set, at);
	}
	if (attr.strikethrough) {
		at = g_new (AtkAttribute, 1);
		at->name = g_strdup ("strikethrough");
		at->value = g_strdup ("true");
		set = g_slist_append (set, at);
	}
	at = g_new (AtkAttribute, 1);
	at->name = g_strdup ("fg-color");
	at->value = g_strdup_printf ("%u,%u,%u",
				     attr.fore.red, attr.fore.green, attr.fore.blue);
	set = g_slist_append (set, at);

	at = g_new (AtkAttribute, 1);
	at->name = g_strdup ("bg-color");
	at->value = g_strdup_printf ("%u,%u,%u",
				     attr.back.red, attr.back.green, attr.back.blue);
	set = g_slist_append (set, at);

	return set;
}

static AtkAttributeSet *
vte_terminal_accessible_get_run_attributes(AtkText *text, gint offset,
					   gint *start_offset, gint *end_offset)
{
	VteTerminalAccessiblePrivate *priv;
	guint i;
	struct _VteCharAttributes cur_attr;
	struct _VteCharAttributes attr;

	vte_terminal_accessible_update_private_data_if_needed(ATK_OBJECT(text),
							      NULL, NULL);

	priv = g_object_get_data(G_OBJECT(text),
				 VTE_TERMINAL_ACCESSIBLE_PRIVATE_DATA);

	attr = g_array_index (priv->snapshot_attributes,
			      struct _VteCharAttributes,
			      offset);
	*start_offset = 0;
	for (i = offset; i--;) {
		cur_attr = g_array_index (priv->snapshot_attributes,
				      struct _VteCharAttributes,
				      i);
		if (!gdk_color_equal (&cur_attr.fore, &attr.fore) ||
		    !gdk_color_equal (&cur_attr.back, &attr.back) ||
		    cur_attr.underline != attr.underline ||
		    cur_attr.strikethrough != attr.strikethrough) {
			*start_offset = i + 1;
			break;
		}
	}
	*end_offset = priv->snapshot_attributes->len - 1;
	for (i = offset + 1; i < priv->snapshot_attributes->len; i++) {
		cur_attr = g_array_index (priv->snapshot_attributes,
				      struct _VteCharAttributes,
				      i);
		if (!gdk_color_equal (&cur_attr.fore, &attr.fore) ||
		    !gdk_color_equal (&cur_attr.back, &attr.back) ||
		    cur_attr.underline != attr.underline ||
		    cur_attr.strikethrough != attr.strikethrough) {
			*end_offset = i - 1;
			break;
		}
	}

	return get_attribute_set (attr);
}

static AtkAttributeSet *
vte_terminal_accessible_get_default_attributes(AtkText *text)
{
	return NULL;
}

static void
vte_terminal_accessible_get_character_extents(AtkText *text, gint offset,
					      gint *x, gint *y,
					      gint *width, gint *height,
					      AtkCoordType coords)
{
	VteTerminalAccessiblePrivate *priv;
	VteTerminal *terminal;
	glong char_width, char_height;
	gint base_x, base_y;

	g_assert(VTE_IS_TERMINAL_ACCESSIBLE(text));

	vte_terminal_accessible_update_private_data_if_needed(ATK_OBJECT(text),
							      NULL, NULL);
	priv = g_object_get_data(G_OBJECT(text),
				 VTE_TERMINAL_ACCESSIBLE_PRIVATE_DATA);
	terminal = VTE_TERMINAL (gtk_accessible_get_widget (GTK_ACCESSIBLE (text)));

	atk_component_get_position (ATK_COMPONENT (text), &base_x, &base_y, coords);
	xy_from_offset (priv, offset, x, y);
	char_width = vte_terminal_get_char_width (terminal);
	char_height = vte_terminal_get_char_height (terminal);
	*x *= char_width;
	*y *= char_height;
	*width = char_width;
	*height = char_height;
	*x += base_x;
	*y += base_y;
}

static gint
vte_terminal_accessible_get_character_count(AtkText *text)
{
	VteTerminalAccessiblePrivate *priv;

	vte_terminal_accessible_update_private_data_if_needed(ATK_OBJECT(text),
							      NULL, NULL);

	priv = g_object_get_data(G_OBJECT(text),
				 VTE_TERMINAL_ACCESSIBLE_PRIVATE_DATA);

	return priv->snapshot_attributes->len;
}

static gint
vte_terminal_accessible_get_offset_at_point(AtkText *text,
					    gint x, gint y,
					    AtkCoordType coords)
{
	VteTerminalAccessiblePrivate *priv;
	VteTerminal *terminal;
	glong char_width, char_height;
	gint base_x, base_y;

	g_assert(VTE_IS_TERMINAL_ACCESSIBLE(text));

	vte_terminal_accessible_update_private_data_if_needed(ATK_OBJECT(text),
							      NULL, NULL);
	priv = g_object_get_data(G_OBJECT(text),
				 VTE_TERMINAL_ACCESSIBLE_PRIVATE_DATA);
	terminal = VTE_TERMINAL (gtk_accessible_get_widget (GTK_ACCESSIBLE (text)));

	atk_component_get_position (ATK_COMPONENT (text), &base_x, &base_y, coords);
	char_width = vte_terminal_get_char_width (terminal);
	char_height = vte_terminal_get_char_height (terminal);
	x -= base_x;
	y -= base_y;
	x /= char_width;
	y /= char_height;
	return offset_from_xy (priv, x, y);
}

static gint
vte_terminal_accessible_get_n_selections(AtkText *text)
{
	GtkWidget *widget;
	VteTerminal *terminal;

	g_assert(VTE_IS_TERMINAL_ACCESSIBLE(text));
	vte_terminal_accessible_update_private_data_if_needed(ATK_OBJECT(text),
							      NULL, NULL);

	widget = gtk_accessible_get_widget (GTK_ACCESSIBLE(text));
	if (widget == NULL) {
		/* State is defunct */
		return -1;
	}
	g_assert(VTE_IS_TERMINAL (widget));
	terminal = VTE_TERMINAL (widget);
	return (vte_terminal_get_has_selection (terminal)) ? 1 : 0;
}

static gchar *
vte_terminal_accessible_get_selection(AtkText *text, gint selection_number,
				      gint *start_offset, gint *end_offset)
{
	GtkWidget *widget;
	VteTerminal *terminal;
	VteTerminalAccessiblePrivate *priv;
	long start_x, start_y, end_x, end_y;

	g_assert(VTE_IS_TERMINAL_ACCESSIBLE(text));
	vte_terminal_accessible_update_private_data_if_needed(ATK_OBJECT(text),
							      NULL, NULL);
	widget = gtk_accessible_get_widget (GTK_ACCESSIBLE(text));
	if (widget == NULL) {
		/* State is defunct */
		return NULL;
	}
	g_assert(VTE_IS_TERMINAL (widget));
	terminal = VTE_TERMINAL (widget);
	if (!vte_terminal_get_has_selection (terminal)) {
		return NULL;
	}
	if (selection_number != 0) {
		return NULL;
	}

	priv = g_object_get_data(G_OBJECT(text),
				 VTE_TERMINAL_ACCESSIBLE_PRIVATE_DATA);
	_vte_terminal_get_start_selection (terminal, &start_x, &start_y);
	*start_offset = offset_from_xy (priv, start_x, start_y);
	_vte_terminal_get_end_selection (terminal, &end_x, &end_y);
	*end_offset = offset_from_xy (priv, end_x, end_y);
	return _vte_terminal_get_selection (terminal);
}

static gboolean
vte_terminal_accessible_add_selection(AtkText *text,
				      gint start_offset, gint end_offset)
{
	GtkWidget *widget;
	VteTerminal *terminal;
	VteTerminalAccessiblePrivate *priv;
	gint start_x, start_y, end_x, end_y;

	g_assert(VTE_IS_TERMINAL_ACCESSIBLE(text));
	vte_terminal_accessible_update_private_data_if_needed(ATK_OBJECT(text),
							      NULL, NULL);
	widget = gtk_accessible_get_widget (GTK_ACCESSIBLE(text));
	if (widget == NULL) {
		/* State is defunct */
		return FALSE;
	}
	g_assert(VTE_IS_TERMINAL (widget));
	terminal = VTE_TERMINAL (widget);
	g_assert(!vte_terminal_get_has_selection (terminal));
	priv = g_object_get_data(G_OBJECT(text),
				 VTE_TERMINAL_ACCESSIBLE_PRIVATE_DATA);
	xy_from_offset (priv, start_offset, &start_x, &start_y);
	xy_from_offset (priv, end_offset, &end_x, &end_y);
	_vte_terminal_select_text (terminal, start_x, start_y, end_x, end_y, start_offset, end_offset);
	return TRUE;
}

static gboolean
vte_terminal_accessible_remove_selection(AtkText *text,
					 gint selection_number)
{
	GtkWidget *widget;
	VteTerminal *terminal;

	g_assert(VTE_IS_TERMINAL_ACCESSIBLE(text));
	vte_terminal_accessible_update_private_data_if_needed(ATK_OBJECT(text),
							      NULL, NULL);
	widget = gtk_accessible_get_widget (GTK_ACCESSIBLE(text));
	if (widget == NULL) {
		/* State is defunct */
		return FALSE;
	}
	g_assert(VTE_IS_TERMINAL (widget));
	terminal = VTE_TERMINAL (widget);
	if (selection_number == 0 && vte_terminal_get_has_selection (terminal)) {
		_vte_terminal_remove_selection (terminal);
		return TRUE;
	} else {
		return FALSE;
	}
}

static gboolean
vte_terminal_accessible_set_selection(AtkText *text, gint selection_number,
				      gint start_offset, gint end_offset)
{
	GtkWidget *widget;
	VteTerminal *terminal;

	g_assert(VTE_IS_TERMINAL_ACCESSIBLE(text));
	vte_terminal_accessible_update_private_data_if_needed(ATK_OBJECT(text),
							      NULL, NULL);
	widget = gtk_accessible_get_widget (GTK_ACCESSIBLE(text));
	if (widget == NULL) {
		/* State is defunct */
		return FALSE;
	}
	g_assert(VTE_IS_TERMINAL (widget));
	terminal = VTE_TERMINAL (widget);
	if (selection_number != 0) {
		return FALSE;
	}
	if (vte_terminal_get_has_selection (terminal)) {
		_vte_terminal_remove_selection (terminal);
	}

	return vte_terminal_accessible_add_selection (text, start_offset, end_offset);
}

static gboolean
vte_terminal_accessible_set_caret_offset(AtkText *text, gint offset)
{
	g_assert(VTE_IS_TERMINAL_ACCESSIBLE(text));
	vte_terminal_accessible_update_private_data_if_needed(ATK_OBJECT(text),
							      NULL, NULL);
	/* Whoa, very not allowed. */
	return FALSE;
}

static void
vte_terminal_accessible_text_init(gpointer iface, gpointer data)
{
	AtkTextIface *text;
	g_assert(G_TYPE_FROM_INTERFACE(iface) == ATK_TYPE_TEXT);
	text = iface;
	_vte_debug_print(VTE_DEBUG_ALLY,
			"Initializing accessible peer's AtkText interface.\n");
	text->get_text = vte_terminal_accessible_get_text;
	text->get_text_after_offset = vte_terminal_accessible_get_text_after_offset;
	text->get_text_at_offset = vte_terminal_accessible_get_text_at_offset;
	text->get_character_at_offset = vte_terminal_accessible_get_character_at_offset;
	text->get_text_before_offset = vte_terminal_accessible_get_text_before_offset;
	text->get_caret_offset = vte_terminal_accessible_get_caret_offset;
	text->get_run_attributes = vte_terminal_accessible_get_run_attributes;
	text->get_default_attributes = vte_terminal_accessible_get_default_attributes;
	text->get_character_extents = vte_terminal_accessible_get_character_extents;
	text->get_character_count = vte_terminal_accessible_get_character_count;
	text->get_offset_at_point = vte_terminal_accessible_get_offset_at_point;
	text->get_n_selections = vte_terminal_accessible_get_n_selections;
	text->get_selection = vte_terminal_accessible_get_selection;
	text->add_selection = vte_terminal_accessible_add_selection;
	text->remove_selection = vte_terminal_accessible_remove_selection;
	text->set_selection = vte_terminal_accessible_set_selection;
	text->set_caret_offset = vte_terminal_accessible_set_caret_offset;
}

static AtkLayer
vte_terminal_accessible_get_layer(AtkComponent *component)
{
	return ATK_LAYER_WIDGET;
}

static gint
vte_terminal_accessible_get_mdi_zorder(AtkComponent *component)
{
	return G_MININT;
}

static gboolean
vte_terminal_accessible_contains(AtkComponent *component,
				 gint x, gint y,
				 AtkCoordType coord_type)
{
	gint ex, ey, ewidth, eheight;
	atk_component_get_extents(component, &ex, &ey, &ewidth, &eheight,
				  coord_type);
	return ((x >= ex) &&
		(x < ex + ewidth) &&
		(y >= ey) &&
		(y < ey + eheight));
}

static void
vte_terminal_accessible_get_extents(AtkComponent *component,
				    gint *x, gint *y,
				    gint *width, gint *height,
				    AtkCoordType coord_type)
{
	atk_component_get_position(component, x, y, coord_type);
	atk_component_get_size(component, width, height);
}

static void
vte_terminal_accessible_get_position(AtkComponent *component,
				     gint *x, gint *y,
				     AtkCoordType coord_type)
{
	GtkWidget *widget;
	*x = 0;
	*y = 0;
	widget = gtk_accessible_get_widget (GTK_ACCESSIBLE(component));
	if (widget == NULL) {
		return;
	}
	if (!gtk_widget_get_realized(widget)) {
		return;
	}
	switch (coord_type) {
	case ATK_XY_SCREEN:
		gdk_window_get_origin(gtk_widget_get_window (widget), x, y);
		break;
	case ATK_XY_WINDOW:
		gdk_window_get_position(gtk_widget_get_window (widget), x, y);
		break;
	default:
		g_assert_not_reached();
		break;
	}
}

static void
vte_terminal_accessible_get_size(AtkComponent *component,
				 gint *width, gint *height)
{
	GtkWidget *widget;
	GdkWindow *window;
	*width = 0;
	*height = 0;
	widget = gtk_accessible_get_widget (GTK_ACCESSIBLE(component));
	if (widget == NULL) {
		return;
	}
	if (!gtk_widget_get_realized(widget)) {
		return;
	}
	window = gtk_widget_get_window (widget);
	if (width)
		*width = gdk_window_get_width (window);
	if (height)
		*height = gdk_window_get_height (window);
}

static gboolean
vte_terminal_accessible_set_extents(AtkComponent *component,
				    gint x, gint y,
				    gint width, gint height,
				    AtkCoordType coord_type)
{
	/* FIXME?  We can change the size, but our position is controlled
	 * by the parent container. */
	return FALSE;
}

static gboolean
vte_terminal_accessible_set_position(AtkComponent *component,
				     gint x, gint y,
				     AtkCoordType coord_type)
{
	/* Controlled by the parent container, if there is one. */
	return FALSE;
}

static gboolean
vte_terminal_accessible_set_size(AtkComponent *component,
				 gint width, gint height)
{
	VteTerminal *terminal;
	gint columns, rows, char_width, char_height;
	GtkWidget *widget;
        GtkBorder *inner_border;

	widget = gtk_accessible_get_widget (GTK_ACCESSIBLE(component));
	if (widget == NULL) {
		return FALSE;
	}
	terminal = VTE_TERMINAL(widget);

        char_width = vte_terminal_get_char_width (terminal);
        char_height = vte_terminal_get_char_height (terminal);
        gtk_widget_style_get (widget, "inner-border", &inner_border, NULL);
	/* If the size is an exact multiple of the cell size, use that,
	 * otherwise round down. */
        columns = (width - (inner_border ? (inner_border->left + inner_border->right) : 0)) / char_width;
        rows = (height - (inner_border ? (inner_border->top + inner_border->bottom) : 0)) / char_height;
        gtk_border_free (inner_border);
	vte_terminal_set_size(terminal, columns, rows);
	return (vte_terminal_get_row_count (terminal) == rows) &&
	       (vte_terminal_get_column_count (terminal) == columns);
}


static AtkObject *
vte_terminal_accessible_ref_accessible_at_point(AtkComponent *component,
						gint x, gint y,
						AtkCoordType coord_type)
{
	/* There are no children. */
	return NULL;
}

static guint
vte_terminal_accessible_add_focus_handler(AtkComponent *component,
					  AtkFocusHandler handler)
{
	guint signal_id;
	signal_id = g_signal_lookup("focus-event",
				    VTE_TYPE_TERMINAL_ACCESSIBLE);
	if (g_signal_handler_find(component,
				  G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_ID,
				  signal_id,
				  0,
				  NULL,
				  (gpointer)handler,
				  NULL) != 0) {
		return 0;
	}
	return g_signal_connect(component, "focus-event",
				G_CALLBACK(handler), NULL);
}

static void
vte_terminal_accessible_remove_focus_handler(AtkComponent *component,
					     guint handler_id)
{
	g_assert(g_signal_handler_is_connected(component, handler_id));
	g_signal_handler_disconnect(component, handler_id);
}

static gboolean
vte_terminal_accessible_grab_focus (AtkComponent *component)
{
	GtkWidget *widget;
	GtkWidget *toplevel;

	widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (component));
	if (!widget)
		return FALSE;

	if (!gtk_widget_get_can_focus (widget))
		return FALSE;

	gtk_widget_grab_focus (widget);
	toplevel = gtk_widget_get_toplevel (widget);
	if (gtk_widget_is_toplevel (toplevel)) {
#ifdef GDK_WINDOWING_X11
		gtk_window_present_with_time (GTK_WINDOW (toplevel),
					      gdk_x11_get_server_time (gtk_widget_get_window (widget)));
#else
		gtk_window_present (GTK_WINDOW (toplevel));
#endif
	}
	return TRUE;
}

static void
vte_terminal_accessible_component_init(gpointer iface, gpointer data)
{
	AtkComponentIface *component;
	g_assert(G_TYPE_FROM_INTERFACE(iface) == ATK_TYPE_COMPONENT);
	component = iface;

	_vte_debug_print(VTE_DEBUG_ALLY,
			"Initializing accessible peer's "
			"AtkComponent interface.\n");
	/* Set our virtual functions. */
	component->add_focus_handler = vte_terminal_accessible_add_focus_handler;
	component->contains = vte_terminal_accessible_contains;
	component->ref_accessible_at_point = vte_terminal_accessible_ref_accessible_at_point;
	component->get_extents = vte_terminal_accessible_get_extents;
	component->get_position = vte_terminal_accessible_get_position;
	component->get_size = vte_terminal_accessible_get_size;
	component->remove_focus_handler = vte_terminal_accessible_remove_focus_handler;
	component->set_extents = vte_terminal_accessible_set_extents;
	component->set_position = vte_terminal_accessible_set_position;
	component->set_size = vte_terminal_accessible_set_size;
	component->get_layer = vte_terminal_accessible_get_layer;
	component->get_mdi_zorder = vte_terminal_accessible_get_mdi_zorder;
	/* everything below copied from gtkwidgetaccessible.c */
	component->grab_focus = vte_terminal_accessible_grab_focus;
}

/* AtkAction interface */

static gboolean
vte_terminal_accessible_do_action (AtkAction *accessible, int i)
{
	GtkWidget *widget;
	gboolean retval = FALSE;

	g_return_val_if_fail (i < LAST_ACTION, FALSE);

	widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (accessible));
	if (!widget) {
		return FALSE;
	}

        switch (i) {
        case ACTION_MENU :
		g_signal_emit_by_name (widget, "popup_menu", &retval);
                break;
        default :
                g_warning ("Invalid action passed to VteTerminalAccessible::do_action");
                return FALSE;
        }
        return retval;
}

static int
vte_terminal_accessible_get_n_actions (AtkAction *accessible)
{
	return LAST_ACTION;
}

static const char *
vte_terminal_accessible_action_get_description (AtkAction *accessible, int i)
{
        VteTerminalAccessiblePrivate *priv;

        g_return_val_if_fail (i < LAST_ACTION, NULL);

	g_return_val_if_fail(VTE_IS_TERMINAL_ACCESSIBLE(accessible), NULL);

	/* Retrieve the private data structure.  It must already exist. */
	priv = g_object_get_data(G_OBJECT(accessible),
				 VTE_TERMINAL_ACCESSIBLE_PRIVATE_DATA);
	g_return_val_if_fail(priv != NULL, NULL);

        if (priv->action_descriptions[i]) {
                return priv->action_descriptions[i];
        } else {
                return vte_terminal_accessible_action_descriptions[i];
        }
}

static const char *
vte_terminal_accessible_action_get_name (AtkAction *accessible, int i)
{
        g_return_val_if_fail (i < LAST_ACTION, NULL);

        return vte_terminal_accessible_action_names[i];
}

static const char *
vte_terminal_accessible_action_get_keybinding (AtkAction *accessible, int i)
{
        g_return_val_if_fail (i < LAST_ACTION, NULL);

        return NULL;
}

static gboolean
vte_terminal_accessible_action_set_description (AtkAction *accessible,
                                                int i,
                                                const char *description)
{
        VteTerminalAccessiblePrivate *priv;

        g_return_val_if_fail (i < LAST_ACTION, FALSE);

	g_return_val_if_fail(VTE_IS_TERMINAL_ACCESSIBLE(accessible), FALSE);

	/* Retrieve the private data structure.  It must already exist. */
	priv = g_object_get_data(G_OBJECT(accessible),
				 VTE_TERMINAL_ACCESSIBLE_PRIVATE_DATA);
	g_return_val_if_fail(priv != NULL, FALSE);

        if (priv->action_descriptions[i]) {
                g_free (priv->action_descriptions[i]);
        }
        priv->action_descriptions[i] = g_strdup (description);

        return TRUE;
}

static void
vte_terminal_accessible_action_init(gpointer iface, gpointer data)
{
	AtkActionIface *action;
	g_return_if_fail(G_TYPE_FROM_INTERFACE(iface) == ATK_TYPE_ACTION);
	action = iface;

	_vte_debug_print(VTE_DEBUG_ALLY,
			"Initializing accessible peer's "
			"AtkAction interface.\n");
	/* Set our virtual functions. */
	action->do_action = vte_terminal_accessible_do_action;
	action->get_n_actions = vte_terminal_accessible_get_n_actions;
	action->get_description = vte_terminal_accessible_action_get_description;
	action->get_name = vte_terminal_accessible_action_get_name;
	action->get_keybinding = vte_terminal_accessible_action_get_keybinding;
	action->set_description = vte_terminal_accessible_action_set_description;
}

static const gchar *
vte_terminal_accessible_get_description (AtkObject *accessible)
{
	GtkWidget *widget;

	widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (accessible));
	if (widget == NULL)
		return NULL;

	if (accessible->description)
		return accessible->description;

	return gtk_widget_get_tooltip_text (widget);
}

static AtkObject *
vte_terminal_accessible_get_parent (AtkObject *accessible)
{
	AtkObject *parent;
	GtkWidget *widget, *parent_widget;

	widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (accessible));
	if (widget == NULL)
		return NULL;

	parent = accessible->accessible_parent;
	if (parent != NULL)
		return parent;

	parent_widget = gtk_widget_get_parent (widget);
	if (parent_widget == NULL)
		return NULL;

	/* For a widget whose parent is a GtkNoteBook, we return the
	 * accessible object corresponding the GtkNotebookPage containing
	 * the widget as the accessible parent.
	 */
	if (GTK_IS_NOTEBOOK (parent_widget)) {
		gint page_num;
		GtkWidget *child;
		GtkNotebook *notebook;

		page_num = 0;
		notebook = GTK_NOTEBOOK (parent_widget);
		while (TRUE) {
			child = gtk_notebook_get_nth_page (notebook, page_num);
			if (!child)
				break;
			if (child == widget) {
				parent = gtk_widget_get_accessible (parent_widget);
				parent = atk_object_ref_accessible_child (parent, page_num);
				g_object_unref (parent);
				return parent;
			}
			page_num++;
		}
	}
	parent = gtk_widget_get_accessible (parent_widget);
	return parent;
}

static gboolean
vte_terminal_accessible_all_parents_visible (GtkWidget *widget)
{
	GtkWidget *iter_parent = NULL;
	gboolean result = TRUE;

	for (iter_parent = gtk_widget_get_parent (widget); iter_parent;
	     iter_parent = gtk_widget_get_parent (iter_parent)) {
		if (!gtk_widget_get_visible (iter_parent)) {
			result = FALSE;
			break;
		}
	}

	return result;
}

static gboolean
vte_terminal_accessible_on_screen (GtkWidget *widget)
{
	GtkAllocation allocation;
	GtkWidget *viewport;
	gboolean return_value;

	gtk_widget_get_allocation (widget, &allocation);

	viewport = gtk_widget_get_ancestor (widget, GTK_TYPE_VIEWPORT);
	if (viewport) {
		GtkAllocation viewport_allocation;
		GtkAdjustment *adjustment;
		GdkRectangle visible_rect;

		gtk_widget_get_allocation (viewport, &viewport_allocation);

		adjustment = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (viewport));
		visible_rect.y = gtk_adjustment_get_value (adjustment);
		adjustment = gtk_scrollable_get_hadjustment (GTK_SCROLLABLE (viewport));
		visible_rect.x = gtk_adjustment_get_value (adjustment);
		visible_rect.width = viewport_allocation.width;
		visible_rect.height = viewport_allocation.height;

		if (((allocation.x + allocation.width) < visible_rect.x) ||
		    ((allocation.y + allocation.height) < visible_rect.y) ||
		    (allocation.x > (visible_rect.x + visible_rect.width)) ||
		    (allocation.y > (visible_rect.y + visible_rect.height)))
			return_value = FALSE;
		else
			return_value = TRUE;
	} else {
		/* Check whether the widget has been placed off the screen.
		 * The widget may be MAPPED as when toolbar items do not
		 * fit on the toolbar.
		 */
		if (allocation.x + allocation.width <= 0 &&
		    allocation.y + allocation.height <= 0)
			return_value = FALSE;
		else
			return_value = TRUE;
	}

	return return_value;
}

static AtkStateSet *
vte_terminal_accessible_ref_state_set (AtkObject *accessible)
{
	GtkWidget *widget;
	AtkStateSet *state_set;

	state_set = ATK_OBJECT_CLASS (vte_terminal_accessible_parent_class)->ref_state_set (accessible);

	widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (accessible));
	if (widget == NULL)
		atk_state_set_add_state (state_set, ATK_STATE_DEFUNCT);
	else {
		if (gtk_widget_is_sensitive (widget)) {
			atk_state_set_add_state (state_set, ATK_STATE_SENSITIVE);
			atk_state_set_add_state (state_set, ATK_STATE_ENABLED);
		}

		if (gtk_widget_get_can_focus (widget)) {
			atk_state_set_add_state (state_set, ATK_STATE_FOCUSABLE);
		}
		/*
		 * We do not currently generate notifications when an ATK object
		 * corresponding to a GtkWidget changes visibility by being scrolled
		 * on or off the screen.  The testcase for this is the main window
		 * of the testgtk application in which a set of buttons in a GtkVBox
		 * is in a scrolled window with a viewport.
		 *
		 * To generate the notifications we would need to do the following:
		 * 1) Find the GtkViewport among the ancestors of the objects
		 * 2) Create an accessible for the viewport
		 * 3) Connect to the value-changed signal on the viewport
		 * 4) When the signal is received we need to traverse the children
		 *    of the viewport and check whether the children are visible or not
		 *    visible; we may want to restrict this to the widgets for which
		 *    accessible objects have been created.
		 * 5) We probably need to store a variable on_screen in the
		 *    GtkWidgetAccessible data structure so we can determine whether
		 *    the value has changed.
		 */
		if (gtk_widget_get_visible (widget)) {
			atk_state_set_add_state (state_set, ATK_STATE_VISIBLE);
			if (vte_terminal_accessible_on_screen (widget) &&
			    gtk_widget_get_mapped (widget) &&
			    vte_terminal_accessible_all_parents_visible (widget))
				atk_state_set_add_state (state_set, ATK_STATE_SHOWING);
		}

		if (gtk_widget_has_focus (widget)) {
			AtkObject *focus_obj;

			focus_obj = g_object_get_data (G_OBJECT (accessible), "gail-focus-object");
			if (focus_obj == NULL)
				atk_state_set_add_state (state_set, ATK_STATE_FOCUSED);
		}

		if (gtk_widget_has_default (widget))
			atk_state_set_add_state (state_set, ATK_STATE_DEFAULT);
	}
	return state_set;
}

static gint
vte_terminal_accessible_get_index_in_parent (AtkObject *accessible)
{
	GtkWidget *widget;
	GtkWidget *parent_widget;
	gint index;
	GList *children;

	widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (accessible));

	if (widget == NULL)
		return -1;

	if (accessible->accessible_parent) {
		AtkObject *parent;

		parent = accessible->accessible_parent;

		if (atk_object_get_role (parent) == ATK_ROLE_PAGE_TAB)
			return 0;
		else {
			gint n_children, i;
			gboolean found = FALSE;

			n_children = atk_object_get_n_accessible_children (parent);
			for (i = 0; i < n_children; i++) {
				AtkObject *child;

				child = atk_object_ref_accessible_child (parent, i);
				if (child == accessible)
					found = TRUE;

				g_object_unref (child);
				if (found)
					return i;
			}
		}
	}

	if (!GTK_IS_WIDGET (widget))
		return -1;
	parent_widget = gtk_widget_get_parent (widget);
	if (!GTK_IS_CONTAINER (parent_widget))
		return -1;

	children = gtk_container_get_children (GTK_CONTAINER (parent_widget));

	index = g_list_index (children, widget);
	g_list_free (children);
	return index;
}

static AtkAttributeSet *
vte_terminal_accessible_get_attributes (AtkObject *obj)
{
	AtkAttributeSet *attributes;
	AtkAttribute *toolkit;

	toolkit = g_new (AtkAttribute, 1);
	toolkit->name = g_strdup ("toolkit");
	toolkit->value = g_strdup ("gtk");

	attributes = g_slist_append (NULL, toolkit);

	return attributes;
}

static void
vte_terminal_accessible_class_init(VteTerminalAccessibleClass *klass)
{
	GObjectClass *gobject_class;
	AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

	vte_terminal_accessible_parent_class = g_type_class_peek_parent (klass);

	gobject_class = G_OBJECT_CLASS(klass);

	class->initialize = vte_terminal_initialize;
	/* Override the finalize method. */
	gobject_class->finalize = vte_terminal_accessible_finalize;

	/* everything below copied from gtkwidgetaccessible.c */
	class->get_description = vte_terminal_accessible_get_description;
	class->get_parent = vte_terminal_accessible_get_parent;
	class->ref_state_set = vte_terminal_accessible_ref_state_set;
	class->get_index_in_parent = vte_terminal_accessible_get_index_in_parent;
	class->get_attributes = vte_terminal_accessible_get_attributes;
}

static void
vte_terminal_accessible_init (VteTerminalAccessible *terminal)
{
}

G_DEFINE_TYPE_WITH_CODE (VteTerminalAccessible, vte_terminal_accessible, GTK_TYPE_ACCESSIBLE,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_TEXT, vte_terminal_accessible_text_init)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_COMPONENT, vte_terminal_accessible_component_init)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_ACTION, vte_terminal_accessible_action_init))

/* Create an accessible peer for the object. */
static AtkObject *
vte_terminal_accessible_factory_create_accessible(GObject *obj)
{
	GtkAccessible *accessible;
	VteTerminal *terminal;

	g_assert(VTE_IS_TERMINAL(obj));

	terminal = VTE_TERMINAL(obj);
	accessible = GTK_ACCESSIBLE(vte_terminal_accessible_new(terminal));
	g_assert(accessible != NULL);

	return ATK_OBJECT(accessible);
}

static void
vte_terminal_accessible_factory_class_init(VteTerminalAccessibleFactoryClass *klass)
{
	AtkObjectFactoryClass *class = ATK_OBJECT_FACTORY_CLASS(klass);
	/* Override the one method we care about. */
	class->create_accessible = vte_terminal_accessible_factory_create_accessible;
}
static void
vte_terminal_accessible_factory_init(VteTerminalAccessibleFactory *self)
{
	/* nothing to initialise */
}

AtkObjectFactory *
vte_terminal_accessible_factory_new(void)
{
	_vte_debug_print(VTE_DEBUG_ALLY,
			"Creating a new VteTerminalAccessibleFactory.\n");
	return g_object_new(VTE_TYPE_TERMINAL_ACCESSIBLE_FACTORY, NULL);
}

