#include "server.h"
#include "ui_server.h"
#include <QtNetwork>
#include <QDebug>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include <fstream>
#define T 3

QTime current_time =QTime::currentTime();

bool operator < (const room &r1, const room &r2){   // 返回true说明r1优先级低于r2
    if(r1.wind_speed==r2.wind_speed){
        return r1.req_time>r2.req_time;
    }
    else return r1.wind_speed<r2.wind_speed;
}

AirCond::AirCond(int slot=1){
    tSlot=slot;
    isOn=false;
    isRoundRobin=false;
    timeCnt=0;
    current=-1;
}

int AirCond::getNext(){
    if(!isOn){
        return -1;
    }
    else if(!isRoundRobin){
        return 0;
    }
    else{
        if(timeCnt==0){   // shift to next AC
            current=(current+1)%shareQ.length();
            timeCnt=1;
            return shareQ.at(current);
        }
        else{
            timeCnt=(timeCnt+1)%tSlot;
            return shareQ.at(current);
        }
    }
}

int AirCond::addShare(int id){
    if(!isOn) isOn=true;
    if(shareQ.length()==0){   // first one
        isOn=true;
        current=0;
    }
    if(isRoundRobin==false && shareQ.length()==1){    // second one
        isRoundRobin=true;
        timeCnt=0;
        current=0;
    }
    shareQ.push_back(id);
    return 0;
}

int AirCond::removeShare(int id){
    int index=0;
    for(QVector<int>::iterator it=shareQ.begin();it!=shareQ.end();it++){
        if(*it==id){
            shareQ.erase(it);
            if(shareQ.length()==0){
                isOn=false;
                current=-1;
            }
            else if(shareQ.length()==1){
                isRoundRobin=false;
                timeCnt=0;
                current=0;
            }
            else{
                current=(index+1)/shareQ.length();
            }
        }
        index++;
    }
    return 0;
}

int AirCond::removeShareIdx(int idx){
    int res=0;
    if(shareQ.length()==0) return -1;
    if(idx>=shareQ.length()) return -1;
    res=shareQ[idx];
    shareQ.remove(idx);
    return res;
}

int AirCond::getNowServicing(){
    if(isOn){
        return shareQ.at(current);
    }
    else return -1;
}

int AirCond::getWillServicing(){
    if(isOn&&isRoundRobin){
        return shareQ[(current+1)%shareQ.length()];
    }
    else return -1;
}

bool AirCond::contains(int id){
    for(QVector<int>::iterator it=shareQ.begin();it!=shareQ.end();it++){
        if(*it==id){
            return true;
        }
    }
    return false;
}

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
    cgtp_pere=0.1;
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

    for(int i=0;i<MAXSERVE;i++){    // init each AC entities
        AirCond* tmp=new AirCond(timeSlot);
        ac.push_back(*tmp);
    }

}

Server::~Server()
{
    delete ui;
}

void Server::connecting()
{
    qDebug()<<"New Connection!";
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
            x += "_" + (room_info[idx].room_id).toUtf8();
            x += "_" + QString("%1").arg(room_info[idx].cur_tp);
            x += "_" + QString("%1").arg(room_info[idx].charge);
            x += "_" + QString::number(current_time.hour())+"/" +QString::number(current_time.minute())+"/"+QString::number(current_time.second());
            x += "_" + QString("%1").arg(room_info[idx].target_tp);
            x += "_" + QString("%1").arg(room_info[idx].wind_speed);
            x += "_" + QString("%1").arg(room_info[idx].change_tp);
            x += "_" + QString("%1").arg(room_info[idx].price_3s);
            x += "_" + QString("%1").arg(room_info[idx].elec);
            break;
        case 2:
        // 2-关机：                    close_房间号
            x = "close";
            x += "_" + (room_info[idx].room_id).toUtf8();
            break;
        case 3:
        // 3-到达目标温度,待机：         sleep_房间号
            x = "sleep";
            x += "_" + (room_info[idx].room_id).toUtf8();
            break;
        case 4:
        // 4-等待：                    wait_房间号_序号
            x = "wait";
            x += "_" + (room_info[idx].room_id).toUtf8();
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
            // todo
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
    for(int i=0; i< sizeof(this->room_info); i++)
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
            room_info[i].cur_tp+=(serve_mod==0?-1:1)*(temp_ecost[wind_rank-1]*T)*cgtp_pere;
            qDebug()<<room_info[i].target_tp-room_info[i].threshold;
            if((serve_mod==0 && room_info[i].cur_tp<=(room_info[i].target_tp-room_info[i].threshold))||
                    (serve_mod==1 && room_info[i].cur_tp>=(room_info[i].target_tp+room_info[i].threshold))){
                send_data(tcpSocket_vec.at(i),3,0);
                qDebug()<<"sleep";
                //todo
                room_info[i].state=SLEEP;
                serve_num--;
            }
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


int Server::put(int room_id, int windspeed){
    // 有空位直接加
    for(int i=0;i<MAXSERVE;i++){
        if(ac.at(i).isOn==false) {
            int tmp=room_id;
            ac[i].addShare(tmp);
            return 0;
        }
    }

    // 没空位找级别低的踢出
    for(int i=0;i<MAXSERVE;i++){
        if(room_info[ac[i].getNowServicing()].wind_speed<windspeed){
            int peer=-1;
            int subwindsp=room_info[ac[i].getNowServicing()].wind_speed;
            for(int j=i+1;j<MAXSERVE;j++){
                if(room_info[ac[j].getNowServicing()].wind_speed==subwindsp){
                    peer=j;
                    break;
                }
            }
            if(peer!=-1){
                while(ac[i].isOn) ac[peer].addShare(ac[i].removeShareIdx(0));

            }
            ac[i].addShare(room_id);
            return 0;
        }
    }

    // 没空位找同级别的加入
    for(int i=0;i<MAXSERVE;i++){
        if(room_info[ac[i].getNowServicing()].wind_speed==windspeed){
            ac[i].addShare(room_id);
            return 0;
        }
    }

    // 全部等级高，加回等待队列中
    request.push(room_info[room_id]);
    return -1;
}

// prior:
// 高级RR时一定没有低级正在服务
// 比他更高级只能单独服务,周围RR一定同级
// 等待队列中一定低一个等级，不可能同级


// Down若变空，只需要轮训周围是否有RR，添加到；若未找到，则从等待队列中添加
// Up删除，之后重新插入
int Server::changeReq(int room_index, int oldSpeed, int newSpeed){

    int idx=-1;     // 空调号
    for(int i=0;i<MAXSERVE;i++){
        if(ac[i].contains(room_index)){
            idx=i;
            break;
        }
    }
    if(idx==-1) return -1;  // not exist
    int found=0;
    if(oldSpeed>newSpeed){    // down
        ac[idx].removeShare(room_index);
        request.push(room_info[idx]);
        if(!ac[idx].isOn){
            for(int i=0;i<MAXSERVE;i++){
                if(ac[i].isRoundRobin){
                    ac[idx].addShare(ac[i].getWillServicing());
                    ac[i].removeShare(ac[i].getWillServicing());
                    found=1;
                }
            }
            if(found==0){
                ac[idx].addShare(request.top().index);
                request.pop();
            }
        }
    }
    else{
        ac[idx].removeShare(room_index);
        if(put(room_index,newSpeed)!=0) qDebug()<<"Error";
    }
}

int Server::updateRequestQueue(){

    int cnt=0;
    while(!request.empty()){
        room tmp=request.top();
        request.pop();
        int state=put(tmp.index,tmp.wind_speed);
        if(state==-1) break;
        cnt++;
    }
    return cnt;
}
