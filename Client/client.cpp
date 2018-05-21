#include "client.h"
#include "ui_client.h"

Client::Client(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Client)
{
    ui->setupUi(this);

    connect(this,SIGNAL(sigModStatus()),this,SLOT(updateStatus()));

}

Client::~Client()
{
    delete ui;
}

// setting
void Client::on_pushButton_7_clicked(){
    double desire_T=ui->lineEdit->text().toDouble();
    int desire_V;
    if(ui->radioButton->isChecked()) desire_V=1;
    else if(ui->radioButton_2->isChecked()) desire_V=2;
    else desire_V=3;

    target_tp=desire_T;
    wind_speed=desire_V;

    //send to server, add record
    QString msg="c_"+to_string(room_id)+"_"+to_string(cur_tp)+"_"+to_string(target_tp)+"_"+to_string(wind_speed);

    send_to_server(msg);
}



int Client::init_connect(){

    return 0;
}

int Client::close_connect(){

    return 0;
}

int Client::send_to_server(QString msg){
    return 0;
}

// 暂停 发送关机命令
void Client::on_pushButton_6_clicked()
{
    QString msg="close_"+to_string(room_id);

    send_to_server(msg);

}

// 开始
void Client::on_pushButton_5_clicked()
{
    target_tp=ui->lineEdit->text().toDouble();
    if(ui->radioButton->isChecked()) wind_speed=1;
    else if(ui->radioButton_2->isChecked()) wind_speed=2;
    else wind_speed=3;

    QString msg="r_"+to_string(room_id)+"_"+to_string(cur_tp)+"_"+tp_string(target_tp)+"_"+to_string(wind_speed);

    send_to_server(msg);

}

void updateInfo(QString msg){

}

double getCur_T(){

}
