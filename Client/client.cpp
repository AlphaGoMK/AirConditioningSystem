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
    this->wind_speed = 0;
    this->room_id = "306D";
    this->target_tp = this->cur_tp;
    this->update();
    QTimer::singleShot(10000, this, SLOT(check_inited()));
    tcp_socket = new QTcpSocket();
    tcp_socket->abort();
    tcp_socket->connectToHost("127.0.0.1", 6666);
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
    this->refresh_screen();
    if(abs(cur_tp-target_tp)>threshold){
        fade_timer->stop();
        QString msg="r_"+room_id+"_"+QString::number(cur_tp)+"_"+QString::number(target_tp)+"_"+QString::number(wind_speed);
        send_to_server(msg);
    }
}

void Client::on_pushButton_7_clicked(){
    double desire_T=ui->get_tg_tp->text().toDouble();
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
            this->t_high = res[0].toDouble();
            this->t_low = res[1].toDouble();
            this->threshold = res[4].toDouble();
            this->state=BOOT;
            this->inited = true;
        }
        else if(res[0] == "a")
        {
            this->cur_tp = res[2].toDouble();
            this->charge = (res[3].toDouble())*0.1;
            //时间暂不处理
            this->target_tp = res[5].toDouble();
            this->wind_speed = res[6].toInt();
            this->state=AT_SERVICE;
            this->refresh_screen();
        }
        else if(res[0] == "sleep"){
            // 开始回温计时器
            fade_timer->start(1000);
            this->state=SLEEP;
            this->refresh_screen();
        }
    }
}

int Client::init(){
    send_to_server(QString("start_" + room_id));
    connect(tcp_socket, SIGNAL(readyRead()), this, SLOT(parse_data()));
    return 0;
}

int Client::init_connect(){
//    if(argc!=3){
//        printf("Usage: %s IPAddress PortNumber/n",argv[0]);
//        exit(-1);
//    }
//    //把字符串的IP地址转化为u_long
//    unsigned long ip;
//    if((ip=inet_addr(argv[1]))==INADDR_NONE){
//        printf("不合法的IP地址：%s",argv[1]);
//        exit(-1);
//    }
//    //把端口号转化成整数
//    short port;
//    if((port = atoi(argv[2]))==0){
//        printf("端口号有误！");
//        exit(-1);
//    }
//    printf("Connecting to %s:%d....../n",inet_ntoa(*(in_addr*)&ip),port);
//    WSADATA wsa;
//    //初始化套接字DLL
//    if(WSAStartup(MAKEWORD(2,2),&wsa)!=0){
//        printf("套接字初始化失败!");
//        exit(-1);
//    }
//    //创建套接字
//    if((sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))==INVALID_SOCKET){
//        printf("创建套接字失败！");
//        exit(-1);
//    }
//    memset(&serverAddress,0,sizeof(sockaddr_in));
//    serverAddress.sin_family=AF_INET;
//    serverAddress.sin_addr.S_un.S_addr = ip;
//    serverAddress.sin_port = htons(port);
//    //建立和服务器的连接
//    if(connect(sock,(sockaddr*)&serverAddress,sizeof(serverAddress))==SOCKET_ERROR){
//        printf("建立连接失败！");
//        exit(-1);
//    }

//    return 0;
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


//int Client::send_to_server(QString msg){
//    QByteArray ba=msg.toLatin1();
//    char* ch=ba.data();
//    char* buf;
//    buf=new char[100];
//    if(send(sock,ch,msg.length(),0)==SOCKET_ERROR){
//        qDebug()<<"发送数据失败！\n";
//        exit(-1);
//    }
//    int bytes;
//    if((bytes=recv(sock,buf,sizeof(buf),0))==SOCKET_ERROR){
//    qDebug()<<"接收数据失败!\n";
//    exit(-1);
//    }
//    buf[bytes]='/0';
//    qDebug()<<"Message from "<<inet_ntoa(serverAddress.sin_addr)<<":"<<buf<<"\n";
//}

void Client::on_pushButton_5_clicked()
{
    target_tp=ui->get_tg_tp->text().toDouble();
    if(ui->radioButton->isChecked()) wind_speed=1;
    else if(ui->radioButton_2->isChecked()) wind_speed=2;
    else wind_speed=3;

    QString msg="r_"+room_id+"_"+QString::number(cur_tp)+"_"+QString::number(target_tp)+"_"+QString::number(wind_speed);

    send_to_server(msg);
}

void Client::on_pushButton_6_clicked()
{
    QString msg="close_"+room_id;

    send_to_server(msg);
}
