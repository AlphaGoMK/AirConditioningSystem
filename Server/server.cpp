#include "server.h"
#include "ui_server.h"
#include <QtNetwork>
#include <QDebug>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include <fstream>
#define T 3

QTime current_time =QTime::currentTime();

Server::Server(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Server)
{
    ui->setupUi(this);

    wait_num=0;
    serve_num=0;
    serve_mod=0;
    ecost[0]=1;
    ecost[1]=2;
    ecost[2]=3;
    eprice=2.0;
    t_high=30.0;
    t_low=16.0;
    MAXSERVE=3;
    tp_range=2.0;
    target_t=target_w=target_x=NULL;

    tcpServer = new QTcpServer(this);
    if(!tcpServer->listen(QHostAddress::Any, 6666))
    {
        qDebug()<<tcpServer->errorString();
        close();
    }
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(connecting()));

//    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(recieve_request()));
//    connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(displayError(QAbstractSocket::SocketError)));

    send_back_timer = new QTimer();                          //新建定时回送计时器, 张尚之加
    send_back_timer->start(1000);                            //定时回送事件，张尚之加
    connect(send_back_timer, SIGNAL(timeout()), this, SLOT(cycleSendBack())); //定时回送事件，张尚之加

    cmp_log_timer = new QTimer();                          //新建定时回送计时器, 张尚之加
    cmp_log_timer->start(300);                            //定时回送事件，张尚之加
    connect(cmp_log_timer, SIGNAL(timeout()), this, SLOT(cycleCompute())); //定时回送事件，张尚之加
}

Server::~Server()
{
    delete ui;
}

void Server::connecting()
{
    QTcpSocket* tcpSocket = tcpServer->nextPendingConnection();
//    connect(tcpSocket, SIGNAL(disconnected()), tcpSocket, SLOT(deleteLater()));
    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(recieve_request()));

    init_room(tcpSocket);
    ui->label->setText("连接成功！");
}

void Server::cycleSendBack()         //定时回送函数，张尚之加
{
    for(int i = 0; i < this->tcpSocket_vec.size(); i++)
    {
        room r = room_info[i];
        if(r.room_id != "" && r.state == AT_SERVICE)
        {
            QTcpSocket* tcpSocket = tcpSocket_vec[i];
            send_data(tcpSocket, 1, 0);
        }
    }
    return;
}

void Server::cycleCompute()
{
    this->compute();
    this->write_log();
}

void Server::send_data(QTcpSocket* tcpSocket, int mod, QString start_id)
{
//    tcpSocket->abort();                                            张尚之注释
//    tcpSocket->connectToHost(QHostAddress::LocalHost, 6666);       张尚之注释
    int idx = this->search_idx(tcpSocket);  //张尚之加
//    target_x = &(room_info[idx]);           //张尚之加
    qDebug() << room_info[idx].room_id;
    QString x;
    switch (mod) {
        case 0:
        // 0-被连接得到start请求，回应：  start_房间号_工作模式_工作温度范围_温度波动范围
            x = "start";
            x += "_" + start_id;     //start_id是什么？
            x += "_" + QString::number(serve_mod);
            x += "_" + QString::number(t_low,'f',2) + "-" + QString::number(t_high,'f',2);
            x += "_" + QString::number(tp_range,'f',2);
            break;
        case 1:
        // 1-服务过程中回应：            a_房间号_当前房间温度_累计计费_timenow_目标温度_风速_温度变化_单价_累计电量
            x = "a";
            x += "_" + (target_x->room_id).toUtf8();
            x += "_" + QString("%1").arg(target_x->cur_tp);
            x += "_" + QString("%1").arg(target_x->charge);
            x += "_" + QString::number(current_time.hour())+"/" +QString::number(current_time.minute())+"/"+QString::number(current_time.second());
            x += "_" + QString("%1").arg(target_x->target_tp);
            x += "_" + QString("%1").arg(target_x->wind_speed);
            x += "_" + QString("%1").arg(target_x->change_tp);
            x += "_" + QString("%1").arg(target_x->price_3s);
            x += "_" + QString("%1").arg(target_x->elec);
            break;
        case 2:
        // 2-关机：                    close_房间号
            x = "close";
            x += "_" + target_x->room_id;
            break;
        case 3:
        // 3-到达目标温度,待机：         sleep_房间号
            x = "sleep";
            x += "_" + target_x->room_id;
            break;
        case 4:
        // 4-等待：                    wait_房间号_序号
            x = "wait";
            x += "_" + target_x->room_id;
            x += "_" + QString::number(wait_num);
            break;
    }
    x += "_$";          //张尚之加
    tcpSocket->write(QByteArray(x.toUtf8()));
    qDebug()<<"Data send:" + x;
}

void Server::recieve_request()
{
    QTcpSocket* tcpSocket = qobject_cast<QTcpSocket*>(sender());
    QString str = tcpSocket->readAll();
    qDebug()<<"recieve: " + str;
    QStringList str_list = str.split('$');   //张尚之加
    int idx = this->search_idx(tcpSocket);    //获取Socket编号， 张尚之加
    for(int i = 0; i < str_list.size(); i++)
    {
        QStringList res = str_list[i].split('_');
        //int closeid;

        if(res[0] == "start")//连接socket：start_房间号
        {
            // ui->messageLabel->setText("审核成功！" + res[1]);
            send_data(tcpSocket, 0, res[1]);

            // initroom(0);                   张尚之删

            //连接后，绑定房间号与tcpSocket_vec、room_info的idx的映射关系 room_id_to_idx[room_id] = idx;
            QString room_id = res[1];
            qDebug() << room_id;
            room_id_to_idx[room_id] = idx;
            room_info[idx].room_id = room_id;
        }
        else if(res[0]=="r")//开机：r_房间号_当前温度_目标温度_风速
        {
            //ui->messageLabel->setText("审核失败！" + res[0]);
//            room_info[idx].room_id=res[1].toInt();
            room_info[idx].cur_tp=res[2].toDouble();
            room_info[idx].target_tp=res[3].toDouble();
            room_info[idx].wind_speed=res[4].toInt();
            room_info[idx].start_time=QString::number(current_time.hour())+"/" +QString::number(current_time.minute())+"/"+QString::number(current_time.second());
            room_info[idx].state=AT_SERVICE;
            serve_num++;
            target_x = &room_info[idx];     //???
        }
        else if(res[0]=="c")//更改：c_房间号_当前温度_目标温度_风速
        {
            findtarget(res[1]);
            if(target_x!=NULL){
                target_x->cur_tp=res[2].toDouble();
                target_x->target_tp=res[3].toDouble();
                target_x->wind_speed=res[4].toInt();
            }
        }
        else if(res[0]=="close")//关机：close_房间号
        {
            findtarget(res[1]);
            if(target_x!=NULL){
                target_x->end_time=QString::number(current_time.hour())+"/" +QString::number(current_time.minute())+"/"+QString::number(current_time.second());
            }
        }

    }
}

void Server::displayError(QAbstractSocket::SocketError)
{
//    qDebug()<<tcpSocket->errorString();
}

void Server::findtarget(QString roomid)
{
    int flg=0;
    for(int i=0; i< this->room_info.size(); i++)
        if(room_info[i].room_id == roomid){
            target_x = &(room_info[i]);
            flg=1;
            break;
        }
    if(!flg)
        target_x=NULL;
}

//void Server::initroom(int i)
//{
//    room_info[i].room_id=-1;
//    room_info[i].cur_tp=room_info[i].change_tp=room_info[i].charge=0;
//    room_info[i].elec=room_info[i].price_3s=0;
//    room_info[i].target_tp=room_info[i].wind_speed=0;
//    room_info[i].t_high=t_high;
//    room_info[i].t_low=t_low;
//    room_info[i].threshold=tp_range;
//    room_info[i].state=CLOSE;
//    room_info[i].start_time="--";
//    room_info[i].end_time="--";
//}

void Server::init_room(QTcpSocket* tcpSocket)
{
    room new_room;
    new_room.room_id="";
    new_room.cur_tp=new_room.change_tp=new_room.charge=0;
    new_room.elec=new_room.price_3s=0;
    new_room.target_tp=new_room.wind_speed=0;
    new_room.t_high=t_high;
    new_room.t_low=t_low;
    new_room.threshold=tp_range;
    new_room.state=CLOSE;
    new_room.start_time="--";
    new_room.end_time="--";
    new_room.tcpSocket = tcpSocket;
    // 同步更新socket与Room 数组
    this->tcpSocket_vec.push_back(tcpSocket);
    this->room_info.push_back(new_room);
}

void Server::write_log(){
    std::fstream log("log.txt", std::ios_base::app);
    int room_num = 5;         //???

    for(int i = 0; i < room_info.size(); i++){
        if(room_info[i].room_id != ""){
//            log << room_info[i].room_id 有bug不会de 先注释掉
            log << " " << room_info[i].t_high
            << " " << room_info[i].t_low
            << " " << room_info[i].threshold
            << " " << (int)room_info[i].state
            << " " << room_info[i].target_tp
            << " " << room_info[i].wind_speed
            << " " << room_info[i].start_time.toStdString()
            << " " << room_info[i].end_time.toStdString()
            << " " << room_info[i].elec
            << " " << room_info[i].charge
            << " " << room_info[i].cur_tp
            << " " << room_info[i].change_tp
            << room_info[i].price_3s
            << endl;
        }

//        if(room_info[i].end_time != "--")         //？？？
//            init_room(i);
    }

    log.close();

}

void Server::compute()
{
//    QVector<room>* temp_room; 张尚之删
    int * temp_ecost;
    int Eprice;

//    temp_room = get_room_arr(); 张尚之删
    temp_ecost=get_ecost();
    Eprice=get_e_price();

    for(int i=0;i<this->room_info.size();i++)
    {
        if(room_info[i].room_id!="" && room_info[i].state==AT_SERVICE)
        {
            int wind_rank=room_info[i].wind_speed;
            room_info[i].elec+=temp_ecost[wind_rank-1]*T;
            room_info[i].charge+=temp_ecost[wind_rank-1]*T*Eprice;
        }
    }
}

/*获取server类私有成员room_info[5]*/
//room * Server::get_room_arr()
//{
//   return room_info;
//}

int Server::search_idx(QTcpSocket *tcpSocket)     //获取tcpSocket对应的编号， 张尚之加
{
    for(int i = 0;  i < this->tcpSocket_vec.size(); i++)
    {
        if(tcpSocket == tcpSocket_vec[i])
        {
            return i;
        }
    }
}

/*获取server类私有成员每种风速下耗电量数组ecost[3]*/
int * Server::get_ecost()
{
    return ecost;
}

QVector<room>* Server::get_room_arr()
{
    return &room_info;
}

/*获取server类私有成员电费*/
float Server::get_e_price()
{
    return eprice;
}
