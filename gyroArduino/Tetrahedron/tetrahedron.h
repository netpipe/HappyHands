#ifndef TETRAHEDRON_H
#define TETRAHEDRON_H

#include <QtOpenGL>
#include <QWidget>
#include <QGLWidget>
#include "glut.h"

namespace Ui {
class Tetrahedron;
}

class Tetrahedron : public QGLWidget
{
    Q_OBJECT
    
public:
    explicit Tetrahedron(QWidget *parent = 0);
    ~Tetrahedron();

protected:
    void initializeGL();
    void resizeGL(int width, int height);
    void paintGL();
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    
private:
    Ui::Tetrahedron *ui;
    void draw();
    int faceAtPosition(const QPoint &pos);

    GLfloat rotationX;
    GLfloat rotationY;
    GLfloat rotationZ;
    QColor faceColors[4];
    QPoint lastPos;
};

#endif // TETRAHEDRON_H
