#ifdef OPENGL
#ifndef OGLWIDGET_H
#define OGLWIDGET_H

#include <QWidget>
#include <QOpenGLWidget>
#include <GL/glu.h>
#include <GL/gl.h>

class OGLWidget : public QOpenGLWidget
{
public:
    OGLWidget(QWidget *parent = 0);
    ~OGLWidget();

protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();
         bool eventFilter( QObject *o, QEvent *e );
};

#endif // OGLWIDGET_H
#endif
