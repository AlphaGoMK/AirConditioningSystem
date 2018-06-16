#include "initwindow.h"
#include "ui_initwindow.h"

InitWindow::InitWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InitWindow)
{
    ui->setupUi(this);
}

InitWindow::~InitWindow()
{
    delete ui;
}

void InitWindow::on_pushButton_clicked()
{
    emit get_env_tp(this->ui->spinBox->value());
    this->close();
}
