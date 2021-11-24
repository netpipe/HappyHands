#include "mainwindow.h"
#include "ui_mainwindow.h"
//#include <oglwidget.h>
#include <arduino.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


#ifdef OPENGL3
  //  widget = new IrrlichtWidget( ui->tabWidget->findChild<QWidget *>("irrRenderWidget0") );
    //QOpenGLWidget
   OGLWidget *widget2 = new OGLWidget(ui->tabWidget->findChild<QWidget *>("openGLWidget"));
   //widget2->(ui->openGLWidget->height());
   // OGLWidget *openGLWidget = new OGLWidget(parent);
  //  openGLWidget->show();
//ui->openGLWidget->resize(300,400);
widget2->resize(ui->openGLWidget->width(),ui->openGLWidget->height());
 //  ui->openGLWidget->showMaximized();
    //     setCentralWidget(widget); //widget
     //    showMaximized();
       //  widget2->autoRepaint();

#endif



}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    main3();
}
