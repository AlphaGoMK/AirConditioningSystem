#include "client.h"
#include "ui_client.h"
#include <QString>
#include <QDebug>
#include <QTimer>
#include <QMessageBox>
#include "initwindow.h"

Client::Client(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Client)
{
    this->initWindow = new InitWindow(this);
    ui->setupUi(this);
    this->inited = false;
    this->cur_tp = 0.;
    this->charge = 0.;
    this->env_tp = -1.0;
    this->target_tp = 0;
    this->wind_speed = 0;
    this->ui->pushButton_2->setDisabled(true);
    this->ui->up->setDisabled(true);

    db=QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("127.0.0.1");
    db.setDatabaseName("air");
    db.setUserName("root");
    db.setPassword("123456");
    // ???
    if(!db.open()){
        qDebug() << "operation error!";
        qDebug() << db.lastError();
        return;
    }
    QSqlQuery query(db);
    QString sqlstr;
    sqlstr="select * from clientparam";
    query.exec(sqlstr);
    query.next();
    this->room_id=query.value(0).toString();
    QString sip=query.value(1).toString();
    int sport=query.value(2).toInt();
    fade_rate=query.value(3).toDouble();


    this->ui->get_tg_tp->display((int)target_tp);
    this->ui->pushButton_7->setDisabled(true);

    QTimer::singleShot(10000, this, SLOT(check_inited()));
    tcp_socket = new QTcpSocket();
    tcp_socket->abort();
    tcp_socket->connectToHost(sip, sport);
    connect(tcp_socket, SIGNAL(readyRead()), this, SLOT(parse_data()));

    this->init();
    fade_timer=new QTimer();
    connect(fade_timer,SIGNAL(timeout()),this,SLOT(fade()));
    connect(initWindow, SIGNAL(get_env_tp(int)), this, SLOT(set_env_tp(int)));




    this->update();
}

Client::~Client()
{
    delete ui;
}

void Client::fade(){
    qDebug() << QString::number(cur_tp);
    cur_tp += (double)(this->mode==COLD?1.0:-1.0) * fade_rate;

    if(this->mode == COLD && cur_tp >= env_tp)
        cur_tp = env_tp;
    if(this->mode == HOT && cur_tp <= env_tp)
        cur_tp = env_tp;

    this->refresh_screen();
    if(abs(cur_tp-target_tp) >= threshold && state == SLEEP && \
            ((this->target_tp < this->cur_tp && this->mode == COLD)||\
             (this->target_tp > this->cur_tp && this->mode == HOT))){  //温差是否达到阈值、状态是否为待机、目标温度是否符合空调模式
        qDebug() << "enter";
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
    if(this->inited==false or this->env_tp == -1.0)
    {
        qDebug() << "Initialization Failed!";
        int ret = QMessageBox::critical(this, "错误", "初始化失败！");
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
        if(res[0] == "start")
        {
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
            if(this->target_tp == 0)     //开机后获取到缺省值
            {
                this->ui->get_tg_tp->display(res[5].toDouble());
                this->ui->up->setDisabled(false);
                this->ui->pushButton_2->setDisabled(false);
            }
            if(this->state != WAIT && this->state != WAIT1 && this->state != WAIT2)
            {
                this->cur_tp = res[2].toDouble();
                this->charge = (res[3].toDouble());
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
            fade_timer->stop();       //空调开始工作，关闭回温函数
        }
        else if(res[0] == "sleep")
        {
            // 开始回温计时器
            if(!fade_timer->isActive())
                fade_timer->start(1000);
            this->state=SLEEP;
        }
        else if(res[0] == "wait")
        {
            if(!fade_timer->isActive())
                fade_timer->start(1000);
            qDebug() << res[3];
            if(res[3] == "1")
                this->state = WAIT1;
            else
                this->state = WAIT2;
        }
        else if(res[0] == "close")
        {
            this->state = BOOT;
            if(!fade_timer->isActive())
                fade_timer->start(1000);
            if(res[2] == "1") //服务器要求退房
            {
                this->reset();
            }
        }
        this->refresh_screen();
    }
}

int Client::init(){
    this->initWindow->show();
    send_to_server(QString("start_" + room_id));
    connect(tcp_socket, SIGNAL(readyRead()), this, SLOT(parse_data()));
    return 0;
}

int Client::close_connect(){
    //清理套接字占用的资源
    WSACleanup();
    return 0;
}

void Client::refresh_screen()
{
    this->ui->cur_tp_lcd->display(int(this->cur_tp + 0.5));
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
        this->ui->label->setText("关机");
        break;
    case AT_SERVICE:
        this->ui->label->setText("工作");
        break;
    case SLEEP:
        this->ui->label->setText("待机");
        break;
    case WAIT:
        this->ui->label->setText("等待");
        break;
    case WAIT1:
        this->ui->label->setText("等待中");
        break;
    case WAIT2:
        this->ui->label->setText("轮转中");
        break;
    default:
        this->ui->label->setText("未连接");
    }
    if(this->state == BOOT)
    {
        this->ui->pushButton_5->setDisabled(false);
        this->ui->pushButton_6->setDisabled(true);
        this->ui->pushButton_7->setDisabled(true);                                                
    }
    else if(state == SLEEP)
    {
        this->ui->pushButton_5->setDisabled(false);
        this->ui->pushButton_6->setDisabled(false);
        this->ui->pushButton_7->setDisabled(true);
    }
    else
    {
        this->ui->pushButton_5->setDisabled(true);
        this->ui->pushButton_6->setDisabled(false);
        this->ui->pushButton_7->setDisabled(false);
    }
    this->update();
}

void Client::reset()
{
    this->wind_speed = 0;
    this->target_tp = 0;
    this->state =  BOOT;
    this->ui->up->setDisabled(true);
    this->ui->pushButton_2->setDisabled(true);
    this->fade_timer->start(1000);    //开启回温
}

int Client::send_to_server(QString msg){
    msg += "_$";
    tcp_socket->write(QByteArray(msg.toUtf8()));
    qDebug() << "Message Sent: " + msg;
    return 0;
}

void Client::on_pushButton_5_clicked()
{
//    if((this->mode == COLD && this->cur_tp <= this->target_tp) || \
//            (this->mode == HOT && this->cur_tp >= this->target_tp))
//    {
//        QString modString = this->mode == COLD? "制冷": "制热";
//        int ret = QMessageBox::critical(this, "错误",QString("当前模式为") + modString);
//    }
//    else
//    {
//        QString msg="r_"+room_id+"_"+QString::number(cur_tp)+"_"+QString::number(target_tp)+"_"+QString::number(wind_speed);
//        send_to_server(msg);
//    }
    if(this->wind_speed == 0)   //以风速为0表示当前为第一次开机，在收到退房的指令时再次置零
    {
        QString msg="r_"+room_id+"_"+QString::number(cur_tp)+"_#" + "_#";
        send_to_server(msg);
    }
    else
    {
        target_tp=(double)(ui->get_tg_tp->intValue());
        if(ui->radioButton->isChecked()) wind_speed=1;
        else if(ui->radioButton_2->isChecked()) wind_speed=2;
        else wind_speed=3;
        QString msg="r_"+room_id+"_"+QString::number(cur_tp)+"_"+QString::number(target_tp)+"_"+QString::number(wind_speed);
        send_to_server(msg);
        this->fade_timer->stop();
    }
}

void Client::on_pushButton_6_clicked()
{
    QString msg="close_"+room_id;

    send_to_server(msg);
}

void Client::on_pushButton_2_clicked()
{
    int tmp = this->ui->get_tg_tp->intValue();
    tmp -= 1;
    if(tmp < this->t_low)
        tmp = t_low;
    this->ui->get_tg_tp->display(tmp);
}

void Client::set_env_tp(int env_tp)
{
    this->env_tp = (double)env_tp;
    this->cur_tp = (double)env_tp;
    this->ui->cur_tp_lcd->display((int)(cur_tp + 0.5));
}

void Client::on_up_clicked()
{
    int tmp = this->ui->get_tg_tp->intValue();
    tmp = tmp + 1;
    if(tmp > this->t_high)
        tmp = t_high;
    this->ui->get_tg_tp->display(tmp);
}
