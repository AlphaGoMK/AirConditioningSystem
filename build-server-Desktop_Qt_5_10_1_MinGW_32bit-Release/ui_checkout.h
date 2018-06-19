/********************************************************************************
** Form generated from reading UI file 'checkout.ui'
**
** Created by: Qt User Interface Compiler version 5.10.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CHECKOUT_H
#define UI_CHECKOUT_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDateTimeEdit>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>

QT_BEGIN_NAMESPACE

class Ui_checkout
{
public:
    QTableWidget *tableWidget;
    QPushButton *search;
    QComboBox *comboBox;
    QDateTimeEdit *starttime;
    QDateTimeEdit *endtime;
    QLabel *label;
    QLabel *label_2;

    void setupUi(QDialog *checkout)
    {
        if (checkout->objectName().isEmpty())
            checkout->setObjectName(QStringLiteral("checkout"));
        checkout->resize(981, 688);
        tableWidget = new QTableWidget(checkout);
        tableWidget->setObjectName(QStringLiteral("tableWidget"));
        tableWidget->setGeometry(QRect(30, 130, 921, 491));
        search = new QPushButton(checkout);
        search->setObjectName(QStringLiteral("search"));
        search->setGeometry(QRect(750, 50, 91, 31));
        comboBox = new QComboBox(checkout);
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->setObjectName(QStringLiteral("comboBox"));
        comboBox->setGeometry(QRect(70, 50, 171, 31));
        QFont font;
        font.setFamily(QStringLiteral("Algerian"));
        font.setPointSize(14);
        font.setBold(false);
        font.setItalic(false);
        font.setWeight(50);
        comboBox->setFont(font);
        comboBox->setStyleSheet(QLatin1String("color:rgb(3, 3, 3);\n"
"background-color: rgb(255, 255, 255);\n"
"font: 14pt \"Algerian\";\n"
""));
        starttime = new QDateTimeEdit(checkout);
        starttime->setObjectName(QStringLiteral("starttime"));
        starttime->setGeometry(QRect(480, 30, 194, 24));
        endtime = new QDateTimeEdit(checkout);
        endtime->setObjectName(QStringLiteral("endtime"));
        endtime->setGeometry(QRect(480, 80, 194, 24));
        label = new QLabel(checkout);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(370, 40, 72, 15));
        label_2 = new QLabel(checkout);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setGeometry(QRect(370, 90, 72, 15));

        retranslateUi(checkout);

        QMetaObject::connectSlotsByName(checkout);
    } // setupUi

    void retranslateUi(QDialog *checkout)
    {
        checkout->setWindowTitle(QApplication::translate("checkout", "Dialog", nullptr));
        search->setText(QApplication::translate("checkout", "\346\237\245\350\257\242", nullptr));
        comboBox->setItemText(0, QApplication::translate("checkout", "\350\257\267\351\200\211\346\213\251\346\210\277\351\227\264\345\217\267", nullptr));
        comboBox->setItemText(1, QApplication::translate("checkout", "306C", nullptr));
        comboBox->setItemText(2, QApplication::translate("checkout", "306D", nullptr));
        comboBox->setItemText(3, QApplication::translate("checkout", "307C", nullptr));
        comboBox->setItemText(4, QApplication::translate("checkout", "307D", nullptr));
        comboBox->setItemText(5, QApplication::translate("checkout", "308C", nullptr));
        comboBox->setItemText(6, QApplication::translate("checkout", "308D", nullptr));
        comboBox->setItemText(7, QApplication::translate("checkout", "309C", nullptr));
        comboBox->setItemText(8, QApplication::translate("checkout", "309D", nullptr));
        comboBox->setItemText(9, QApplication::translate("checkout", "310C", nullptr));
        comboBox->setItemText(10, QApplication::translate("checkout", "310D", nullptr));

        label->setText(QApplication::translate("checkout", "\350\265\267\345\247\213\346\227\266\351\227\264", nullptr));
        label_2->setText(QApplication::translate("checkout", "\347\273\210\346\255\242\346\227\266\351\227\264", nullptr));
    } // retranslateUi

};

namespace Ui {
    class checkout: public Ui_checkout {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CHECKOUT_H
