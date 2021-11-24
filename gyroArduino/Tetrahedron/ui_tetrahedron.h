/********************************************************************************
** Form generated from reading UI file 'tetrahedron.ui'
**
** Created by: Qt User Interface Compiler version 5.12.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TETRAHEDRON_H
#define UI_TETRAHEDRON_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Tetrahedron
{
public:

    void setupUi(QWidget *Tetrahedron)
    {
        if (Tetrahedron->objectName().isEmpty())
            Tetrahedron->setObjectName(QString::fromUtf8("Tetrahedron"));
        Tetrahedron->resize(400, 300);

        retranslateUi(Tetrahedron);

        QMetaObject::connectSlotsByName(Tetrahedron);
    } // setupUi

    void retranslateUi(QWidget *Tetrahedron)
    {
        Tetrahedron->setWindowTitle(QApplication::translate("Tetrahedron", "Tetrahedron", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Tetrahedron: public Ui_Tetrahedron {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TETRAHEDRON_H
