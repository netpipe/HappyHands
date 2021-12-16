#-------------------------------------------------
#
# Project created by QtCreator 2013-12-24T13:02:31
#
#-------------------------------------------------

QT       += core gui
QT       += opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Tetrahedron
TEMPLATE = app


SOURCES += main.cpp\
        tetrahedron.cpp

HEADERS  += tetrahedron.h \
    arduino/arduino.h

FORMS    += tetrahedron.ui

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/ -lglut32
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/ -lglut32d
else:unix: LIBS += -L$$PWD/ -lglut -lGLU

INCLUDEPATH += $$PWD/
DEPENDPATH += $$PWD/
