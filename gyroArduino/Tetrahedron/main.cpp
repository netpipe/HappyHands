#include "tetrahedron.h"
#include <QApplication>
#include<iostream>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if (!QGLFormat::hasOpenGL()) {
        std::cerr << "This system has no OpenGL support" << std::endl;
        return 1;
    }

    Tetrahedron w;
    w.setWindowTitle(QObject::tr("Tetrahedron"));
    w.resize(300, 300);
    w.show();
    
    return a.exec();
}
