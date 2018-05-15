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
    if(argc!=3){
        printf("Usage: %s IPAddress PortNumber/n",argv[0]);
        exit(-1);
    }
    //把字符串的IP地址转化为u_long
    unsigned long ip;
    if((ip=inet_addr(argv[1]))==INADDR_NONE){
        printf("不合法的IP地址：%s",argv[1]);
        exit(-1);
    }
    //把端口号转化成整数
    short port;
    if((port = atoi(argv[2]))==0){
        printf("端口号有误！");
        exit(-1);
    }
    printf("Connecting to %s:%d....../n",inet_ntoa(*(in_addr*)&ip),port);
    WSADATA wsa;
    //初始化套接字DLL
    if(WSAStartup(MAKEWORD(2,2),&wsa)!=0){
        printf("套接字初始化失败!");
        exit(-1);
    }
    //创建套接字
    if((sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))==INVALID_SOCKET){
        printf("创建套接字失败！");
        exit(-1);
    }
    memset(&serverAddress,0,sizeof(sockaddr_in));
    serverAddress.sin_family=AF_INET;
    serverAddress.sin_addr.S_un.S_addr = ip;
    serverAddress.sin_port = htons(port);
    //建立和服务器的连接
    if(connect(sock,(sockaddr*)&serverAddress,sizeof(serverAddress))==SOCKET_ERROR){
        printf("建立连接失败！");
        exit(-1);
    }

    return 0;
}

int Client::close_connect(){
    //清理套接字占用的资源
    WSACleanup();
    return 0;
}

int Client::send_to_server(QString msg){
    QByteArray ba=msg.toLatin1();
    char* ch=ba.data();
    char* buf;
    buf=new char[100];
    if(send(sock,ch,msg.length(),0)==SOCKET_ERROR){
        QDebug()<<"发送数据失败！\n";
        exit(-1);
    }
    int bytes;
    if((bytes=recv(sock,buf,sizeof(buf),0))==SOCKET_ERROR){
    QDebug()<<"接收数据失败!\n";
    exit(-1);
    }
    buf[bytes]='/0';
    QDebug()<<"Message from "<<inet_ntoa(serverAddress.sin_addr)<<":"<<buf<<"\n";
}
