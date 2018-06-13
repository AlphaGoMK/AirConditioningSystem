#include "client.h"
#include "ui_client.h"
#include <QString>
#include <QDebug>
#include <QTimer>
#include <QMessageBox>

Client::Client(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Client)
{
    ui->setupUi(this);
    this->inited = false;
    this->cur_tp = 28;
    this->charge = 0.;
    this->env_tp = 28;
    this->wind_speed = 0;
    this->room_id = "306C";
    this->target_tp = this->cur_tp;
    this->ui->get_tg_tp->display(30);
    this->update();
    QTimer::singleShot(10000, this, SLOT(check_inited()));
    tcp_socket = new QTcpSocket();
    tcp_socket->abort();
    tcp_socket->connectToHost("10.105.240.188", 6666);
    connect(tcp_socket, SIGNAL(readyRead()), this, SLOT(parse_data()));

    this->init();
    fade_rate=0.5;
    fade_timer=new QTimer();
    connect(fade_timer,SIGNAL(timeout()),this,SLOT(fade()));

//    connect(this,SIGNAL(sigModStatus()),this,SLOT(updateStatus()));

}

Client::~Client()
{
    delete ui;
}

void Client::fade(){
    cur_tp+=(this->mode==COLD?1:-1)*fade_rate;

    if(this->mode == COLD && cur_tp >= env_tp)
        cur_tp = env_tp;
    if(this->mode == HOT && cur_tp <= env_tp)
        cur_tp = env_tp;

    this->refresh_screen();
    if(abs(cur_tp-target_tp)>threshold && state != CLOSE){
        fade_timer->stop();
        QString msg="r_"+room_id+"_"+QString::number(cur_tp)+"_"+QString::number(target_tp)+"_"+QString::number(wind_speed);
        send_to_server(msg);
    }
}

void Client::on_pushButton_7_clicked(){
    double desire_T=(double)(ui->get_tg_tp->intValue());
    int desire_V;
    if(ui->radioButton->isChecked()) desire_V=1;
    else if(ui->radioButton_2->isChecked()) desire_V=2;
    else desire_V=3;

    target_tp=desire_T;
    wind_speed=desire_V;

    //send to server, add record
    QString msg = "c_" +room_id+"_"+QString::number(cur_tp)+"_"+QString::number(target_tp)+"_"+QString::number(wind_speed);
    qDebug() << "msg";
    send_to_server(msg);
}

void Client::updateStatus()
{

}

void Client::check_inited()   //初始化失败，弹窗提示
{
    if(this->inited==false)
    {
        qDebug() << "Initialization Failed!";
        int ret = QMessageBox::critical(this, "错误", "连接服务器失败！");
        this->close();
    }
    else
    {
        qDebug() << "Successfully Initialized!";
    }
    return;
}

void Client::parse_data()
{
    QString str = tcp_socket->readAll();
    if(str == "")
        return;
    qDebug()<<"Data Recieved:" << str;
    QStringList str_list = str.split('$');
    for(int i=0; i < str_list.size(); i++)
    {
        str = str_list[i];
        if(!(str[0] >= 'a' && str[0] <= 'z'))
            break;
        QStringList res = str.split('_');
        if(res[0] == "start"){
            if(res[2] == "0")
            {
                this->mode = COLD;
            }
            else
            {
                this->mode = HOT;
            }
            QStringList t_range = res[3].split('-');
            this->t_high = t_range[1].toDouble();
            this->t_low = t_range[0].toDouble();
            this->threshold = res[4].toDouble();
            this->state=BOOT;
            this->inited = true;
        }
        else if(res[0] == "a")
        {
            this->state = AT_SERVICE;
            if(this->state != WAIT)
            {
                this->cur_tp = res[2].toDouble();
                this->charge = (res[3].toDouble())*0.1;
                //时间暂不处理
                this->target_tp = res[5].toDouble();
                this->wind_speed = res[6].toInt();
                this->state=AT_SERVICE;
            }
            else
            {
                //从WAIT状态重新转到AT_SERVICE
                QString msg="c_"+room_id+"_"+QString::number(cur_tp)+"_"+QString::number(target_tp)+"_"+QString::number(wind_speed);
                send_to_server(msg);
            }
        }
        else if(res[0] == "sleep")
        {
            // 开始回温计时器
            fade_timer->start(1000);
            this->state=SLEEP;
        }
        else if(res[0] == "wait")
        {
            fade_timer->start(1000);
            this->state = WAIT;
        }
        else if(res[0] == "close")
        {
            fade_timer->start(1000);
            this->state = CLOSE;
        }
        this->refresh_screen();
    }
}

int Client::init(){
    send_to_server(QString("start_" + room_id));
    connect(tcp_socket, SIGNAL(readyRead()), this, SLOT(parse_data()));
    return 0;
}

int Client::init_connect()
{
}

int Client::close_connect(){
    //清理套接字占用的资源
    WSACleanup();
    return 0;
}

void Client::refresh_screen()
{
    this->ui->cur_tp_lcd->display(int(this->cur_tp));
    this->ui->tg_tp_lcd->display(int(this->target_tp));
    switch(this->wind_speed)
    {
    case 1:
        this->ui->wspd_label->setText("低");
        break;
    case 2:
        this->ui->wspd_label->setText("中");
        break;
    case 3:
        this->ui->wspd_label->setText("高");
        break;
    default:
        this->ui->wspd_label->setText("");
        break;
    }
    if(mode == COLD)
    {
        this->ui->modeLabel->setText("制冷");
    }
    else
    {
        this->ui->modeLabel->setText("制热");
    }
    if (state!=AT_SERVICE){
        this->ui->wspd_label->setText("");
    }
    this->ui->chg_label->setText(QString::number(this->charge, 'g', 2));
    switch(state){
    case BOOT:
        this->ui->label->setText("启动");
        break;
    case AT_SERVICE:
        this->ui->label->setText("工作");
        break;
    case SLEEP:
        this->ui->label->setText("完成");
        break;
    case WAIT:
        this->ui->label->setText("等待");
        break;
    default:
        this->ui->label->setText("关闭");
    }
    this->update();
}

int Client::send_to_server(QString msg){
    tcp_socket->write(QByteArray(msg.toUtf8()));
    qDebug() << "Message Sent: " + msg;
    return 0;
}

void Client::on_pushButton_5_clicked()
{
    target_tp=(double)(ui->get_tg_tp->intValue());
    if(ui->radioButton->isChecked()) wind_speed=1;
    else if(ui->radioButton_2->isChecked()) wind_speed=2;
    else wind_speed=3;
    if((this->mode == COLD && this->cur_tp <= this->target_tp) || \
            (this->mode == HOT && this->cur_tp >= this->target_tp))
    {
        QString modString = this->mode == COLD? "制冷": "制热";
        int ret = QMessageBox::critical(this, "错误",QString("当前模式为") + modString);
    }
    else
    {
        QString msg="r_"+room_id+"_"+QString::number(cur_tp)+"_"+QString::number(target_tp)+"_"+QString::number(wind_speed);
        send_to_server(msg);
    }
}

void Client::on_pushButton_6_clicked()
{
    QString msg="close_"+room_id;

    send_to_server(msg);
}

void Client::on_pushButton_clicked()
{
    int tmp = this->ui->get_tg_tp->intValue();
    tmp = tmp + 1;
    if(tmp > this->t_high)
        tmp = t_high;
    this->ui->get_tg_tp->display(tmp);
}

void Client::on_pushButton_2_clicked()
{
    int tmp = this->ui->get_tg_tp->intValue();
    tmp -= 1;
    if(tmp < this->t_low)
        tmp = t_low;
    this->ui->get_tg_tp->display(tmp);
}
