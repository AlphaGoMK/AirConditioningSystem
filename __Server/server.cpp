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

int AirCond::popback()
{
    int res=shareQ.back();
    shareQ.pop_back();
    return res;
}

Server::Server(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Server)
{
    ui->setupUi(this);

    wait_num=0;
    serve_num=0;
    serve_mod=0; // 0制冷 1制热
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

//    send_back_timer = new QTimer();                          //新建定时回送计时器, 张尚之加
//    send_back_timer->start(1000);                            //定时回送事件，张尚之加
//    connect(send_back_timer, SIGNAL(timeout()), this, SLOT(cycleSendBack())); //定时回送事件，张尚之加

    cmp_log_timer = new QTimer();                          //新建定时回送计时器, 张尚之加
    cmp_log_timer->start(1000);                            //定时回送事件，张尚之加
    connect(cmp_log_timer, SIGNAL(timeout()), this, SLOT(cycleCompute())); //定时回送事件，张尚之加

    for(int i=0;i<MAXSERVE;i++){    // init each AC entities
        AirCond* tmp=new AirCond(timeSlot);
        ac.push_back(*tmp);
    }

    this->idToIdx["306C"] = 0;
    this->idToIdx["306D"] = 1;
    this->idToIdx["307C"] = 2;
    this->idToIdx["307D"] = 3;
    this->idToIdx["308C"] = 4;
    this->idToIdx["308D"] = 5;
    this->idToIdx["309C"] = 6;
    this->idToIdx["309D"] = 7;
    this->idToIdx["310C"] = 8;
    this->idToIdx["310D"] = 9;

    this->refreshInfoWindow();
}

Server::~Server()
{
    delete ui;
}

void Server::connecting()
{
    qDebug()<<"New Connection!";
    QTcpSocket* tcpSocket = tcpServer->nextPendingConnection();
    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(recieve_request()));

    init_room(tcpSocket);
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
    this->cycleSendBack();
    this->refreshInfoWindow();
    this->write_log();
}

void Server::send_data(QTcpSocket* tcpSocket, int mod, QString start_id)
{
    int idx = this->search_idx(tcpSocket);  //张尚之加
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
            send_data(tcpSocket, 0, res[1]);

            //连接后，绑定房间号与tcpSocket_vec、room_info的idx的映射关系 room_id_to_idx[room_id] = idx;
            QString room_id = res[1];
            qDebug() << room_id;
            room_id_to_idx[room_id] = idx;
            room_info[idx].room_id = room_id;
        }
        else if(res[0]=="r")//开机：r_房间号_当前温度_目标温度_风速
        {
            room_info[idx].cur_tp=res[2].toDouble();
            if(res[3] == "#")
            {
               room_info[idx].target_tp = this->serve_mod == 0? this->default_tg_tmp_low:this->default_tg_tmp_low;
            }
            else
            {
                room_info[idx].target_tp = res[3].toDouble();
            }
            if(res[4] == "#")
            {
                room_info[idx].wind_speed = this->default_fan;
            }
            else
            {
                room_info[idx].wind_speed = res[4].toInt();
            }
            room_info[idx].req_time = QTime::currentTime();
            room_info[idx].index = idx;
            int req_state = this->put(idx, room_info[idx].wind_speed);
            if(req_state == -1)
            {
                send_data(tcpSocket, 3, 0);
                room_info[idx].state = WAIT;
            }
            else
            {
                room_info[idx].state = AT_SERVICE;
            }
        }
        else if(res[0]=="c")//更改：c_房间号_当前温度_目标温度_风速
        {
            //todo
//            if(target_x!=NULL){
//                target_x->cur_tp=res[2].toDouble();
//                target_x->target_tp=res[3].toDouble();
//                target_x->wind_speed=res[4].toInt();
//            }
        }
        else if(res[0]=="close")//关机：close_房间号
        {
            if(target_x!=NULL){
                target_x->end_time=QString::number(current_time.hour())+"/" +QString::number(current_time.minute())+"/"+QString::number(current_time.second());
            }
            room_info[idx].state = CLOSE;

            send_data(tcpSocket, 2, 0);
        }
    }
}

void Server::displayError(QAbstractSocket::SocketError)
{
//    qDebug()<<tcpSocket->errorString();
}

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
//    int * temp_ecost;
//    int Eprice;

//    temp_ecost=get_ecost();
//    Eprice=get_e_price();

//    for(int i = 0; i < MAXSERVE; i++)
//    {
//        AirCond cur_ac = ac[i];
//        if(cur_ac.isOn)
//        {
//            int idx = cur_ac.getNowServicing();
//            int wind_rank=room_info[i].wind_speed;
//            room_info[idx].elec+=temp_ecost[wind_rank-1]*T;
//            room_info[idx].charge+=temp_ecost[wind_rank-1]*T*Eprice;
//            room_info[idx].cur_tp+=(serve_mod==0?-1:1)*(temp_ecost[wind_rank-1]*T)*cgtp_pere;
//            if((serve_mod==0 && room_info[idx].cur_tp<=room_info[idx].target_tp)||
//                    (serve_mod==1 && room_info[idx].cur_tp>=room_info[idx].target_tp))
//            {
//                send_data(tcpSocket_vec.at(idx),3,0);
//                qDebug()<<"sleep";
//                //todo
//                room_info[idx].state=SLEEP;
//                ac[i].removeShare(idx);
//            }
//            ac[i].getNext();
//        }
//        else
//        {
//            continue;
//        }
//    }
}


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
        ac[idx].removeShare(room_index);    // 删除，放到请求队列中
        request.push(room_info[idx]);
        if(!ac[idx].isOn){                  // 减到空
            for(int i=0;i<MAXSERVE;i++){    // 调整
                if(ac[i].isRoundRobin){
                    ac[idx].addShare(ac[i].getWillServicing());
                    ac[i].removeShare(ac[i].getWillServicing());
                    break;
                }
            }
            // 重新添加
            while(put(request.top().index,request.top().wind_speed)!=-1){
                request.pop();
            }
        }
    }
    else{   // 增加，删除，重新添加平衡
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

int Server::balanceAC()
{
    // fill the blank
    int flag=0;
    for(int i=0;i<MAXSERVE;i++){
        if(!ac[i].isOn){    // find largest RR to fill
            int maxRR=0;
            for(int j=0;j<MAXSERVE;j++){
                if(ac[j].isRoundRobin&&ac[j].getShareQ().length()>ac[maxRR].getshareQ().length())
                    maxRR=j;
            }
            if(ac[maxRR].isRoundRobin){ // found
                int num=ac[maxRR].getShareQ().length();
                num/=2;
                for(int k=0;k<num;k++){
                    int back=ac[maxRR].popback();
                    ac[i].addShare(back);
                }
            }
            else{   // 全是独占
                if(request.empty()) return 0;   // 没有更多请求
                while(put(request.top()->index,request.top()->wind_speed)!=-1) request.pop();   // 添加请求
            }
        }
    }

    return 0;
}

//遍历room_info，刷新界面信息
void Server::refreshInfoWindow()
{
    bool modified[10] = {false};
    for(int i = 0; i < this->room_info.size(); i ++)
    {
        room* curRoom = &(this->room_info[i]);
        int idx = this->idToIdx[curRoom->room_id];
        modified[idx] = true;
        switch(idx)
        {
        case 0:
            this->ui->stateLabel_0->setText(stateToString(curRoom->state));
            this->ui->curTmpLabel_0->setText(QString::number((int)curRoom->cur_tp));
            this->ui->tgTmpLabel_0->setText(QString::number((int)curRoom->target_tp));
            this->ui->elecLabel_0->setText(QString::number(curRoom->elec, 'f', 2));
            this->ui->chargeLabel_0->setText(QString::number(curRoom->charge, 'f', 2));
            this->ui->fanLabel_0->setText(fanToString(curRoom->wind_speed));
            break;
        case 1:
            this->ui->stateLabel_1->setText(stateToString(curRoom->state));
            this->ui->curTmpLabel_1->setText(QString::number((int)curRoom->cur_tp));
            this->ui->tgTmpLabel_1->setText(QString::number((int)curRoom->target_tp));
            this->ui->elecLabel_1->setText(QString::number(curRoom->elec, 'f', 2));
            this->ui->chargeLabel_1->setText(QString::number(curRoom->charge, 'f', 2));
            this->ui->fanLabel_1->setText(fanToString(curRoom->wind_speed));
            break;
        case 2:
            this->ui->stateLabel_2->setText(stateToString(curRoom->state));
            this->ui->curTmpLabel_2->setText(QString::number((int)curRoom->cur_tp));
            this->ui->tgTmpLabel_2->setText(QString::number((int)curRoom->target_tp));
            this->ui->elecLabel_2->setText(QString::number(curRoom->elec, 'f', 2));
            this->ui->chargeLabel_2->setText(QString::number(curRoom->charge, 'f', 2));
            this->ui->fanLabel_2->setText(fanToString(curRoom->wind_speed));
            break;
        case 3:
            this->ui->stateLabel_3->setText(stateToString(curRoom->state));
            this->ui->curTmpLabel_3->setText(QString::number((int)curRoom->cur_tp));
            this->ui->tgTmpLabel_3->setText(QString::number((int)curRoom->target_tp));
            this->ui->elecLabel_3->setText(QString::number(curRoom->elec, 'f', 2));
            this->ui->chargeLabel_3->setText(QString::number(curRoom->charge, 'f', 2));
            this->ui->fanLabel_3->setText(fanToString(curRoom->wind_speed));
            break;
        case 4:
            this->ui->stateLabel_4->setText(stateToString(curRoom->state));
            this->ui->curTmpLabel_4->setText(QString::number((int)curRoom->cur_tp));
            this->ui->tgTmpLabel_4->setText(QString::number((int)curRoom->target_tp));
            this->ui->elecLabel_4->setText(QString::number(curRoom->elec, 'f', 2));
            this->ui->chargeLabel_4->setText(QString::number(curRoom->charge, 'f', 2));
            this->ui->fanLabel_4->setText(fanToString(curRoom->wind_speed));
            break;
        case 5:
            this->ui->stateLabel_5->setText(stateToString(curRoom->state));
            this->ui->curTmpLabel_5->setText(QString::number((int)curRoom->cur_tp));
            this->ui->tgTmpLabel_5->setText(QString::number((int)curRoom->target_tp));
            this->ui->elecLabel_5->setText(QString::number(curRoom->elec, 'f', 2));
            this->ui->chargeLabel_5->setText(QString::number(curRoom->charge, 'f', 2));
            this->ui->fanLabel_5->setText(fanToString(curRoom->wind_speed));
            break;
        case 6:
            this->ui->stateLabel_6->setText(stateToString(curRoom->state));
            this->ui->curTmpLabel_6->setText(QString::number((int)curRoom->cur_tp));
            this->ui->tgTmpLabel_6->setText(QString::number((int)curRoom->target_tp));
            this->ui->elecLabel_6->setText(QString::number(curRoom->elec, 'f', 2));
            this->ui->chargeLabel_6->setText(QString::number(curRoom->charge, 'f', 2));
            this->ui->fanLabel_6->setText(fanToString(curRoom->wind_speed));
            break;
        case 7:
            this->ui->stateLabel_7->setText(stateToString(curRoom->state));
            this->ui->curTmpLabel_7->setText(QString::number((int)curRoom->cur_tp));
            this->ui->tgTmpLabel_7->setText(QString::number((int)curRoom->target_tp));
            this->ui->elecLabel_7->setText(QString::number(curRoom->elec, 'f', 2));
            this->ui->chargeLabel_7->setText(QString::number(curRoom->charge, 'f', 2));
            this->ui->fanLabel_7->setText(fanToString(curRoom->wind_speed));
            break;
        case 8:
            this->ui->stateLabel_8->setText(stateToString(curRoom->state));
            this->ui->curTmpLabel_8->setText(QString::number((int)curRoom->cur_tp));
            this->ui->tgTmpLabel_8->setText(QString::number((int)curRoom->target_tp));
            this->ui->elecLabel_8->setText(QString::number(curRoom->elec, 'f', 2));
            this->ui->chargeLabel_8->setText(QString::number(curRoom->charge, 'f', 2));
            this->ui->fanLabel_8->setText(fanToString(curRoom->wind_speed));
            break;
        case 9:
            this->ui->stateLabel_9->setText(stateToString(curRoom->state));
            this->ui->curTmpLabel_9->setText(QString::number((int)curRoom->cur_tp));
            this->ui->tgTmpLabel_9->setText(QString::number((int)curRoom->target_tp));
            this->ui->elecLabel_9->setText(QString::number(curRoom->elec, 'f', 2));
            this->ui->chargeLabel_9->setText(QString::number(curRoom->charge, 'f', 2));
            this->ui->fanLabel_9->setText(fanToString(curRoom->wind_speed));
            break;
        default:
            break;
        }
    }
    for(int i = 0; i < 10; i++)
    {
        if(modified[i] == false)
        {
            switch(i)
            {
            case 0:
                this->ui->stateLabel_0->setText("未连接");
                this->ui->curTmpLabel_0->setText("");
                this->ui->tgTmpLabel_0->setText("");
                this->ui->elecLabel_0->setText("");
                this->ui->chargeLabel_0->setText("");
                this->ui->fanLabel_0->setText("");
                break;
            case 1:
                this->ui->stateLabel_1->setText("未连接");
                this->ui->curTmpLabel_1->setText("");
                this->ui->tgTmpLabel_1->setText("");
                this->ui->elecLabel_1->setText("");
                this->ui->chargeLabel_1->setText("");
                this->ui->fanLabel_1->setText("");
                break;
            case 2:
                this->ui->stateLabel_2->setText("未连接");
                this->ui->curTmpLabel_2->setText("");
                this->ui->tgTmpLabel_2->setText("");
                this->ui->elecLabel_2->setText("");
                this->ui->chargeLabel_2->setText("");
                this->ui->fanLabel_2->setText("");
                break;
            case 3:
                this->ui->stateLabel_3->setText("未连接");
                this->ui->curTmpLabel_3->setText("");
                this->ui->tgTmpLabel_3->setText("");
                this->ui->elecLabel_3->setText("");
                this->ui->chargeLabel_3->setText("");
                this->ui->fanLabel_3->setText("");
                break;
            case 4:
                this->ui->stateLabel_4->setText("未连接");
                this->ui->curTmpLabel_4->setText("");
                this->ui->tgTmpLabel_4->setText("");
                this->ui->elecLabel_4->setText("");
                this->ui->chargeLabel_4->setText("");
                this->ui->fanLabel_4->setText("");
                break;
            case 5:
                this->ui->stateLabel_5->setText("未连接");
                this->ui->curTmpLabel_5->setText("");
                this->ui->tgTmpLabel_5->setText("");
                this->ui->elecLabel_5->setText("");
                this->ui->chargeLabel_5->setText("");
                this->ui->fanLabel_5->setText("");
                break;
            case 6:
                this->ui->stateLabel_6->setText("未连接");
                this->ui->curTmpLabel_6->setText("");
                this->ui->tgTmpLabel_6->setText("");
                this->ui->elecLabel_6->setText("");
                this->ui->chargeLabel_6->setText("");
                this->ui->fanLabel_6->setText("");
                break;
            case 7:
                this->ui->stateLabel_7->setText("未连接");
                this->ui->curTmpLabel_7->setText("");
                this->ui->tgTmpLabel_7->setText("");
                this->ui->elecLabel_7->setText("");
                this->ui->chargeLabel_7->setText("");
                this->ui->fanLabel_7->setText("");
                break;
           case 8:
                this->ui->stateLabel_8->setText("未连接");
                this->ui->curTmpLabel_8->setText("");
                this->ui->tgTmpLabel_8->setText("");
                this->ui->elecLabel_8->setText("");
                this->ui->chargeLabel_8->setText("");
                this->ui->fanLabel_8->setText("");
                break;
            case 9:
                this->ui->stateLabel_9->setText("未连接");
                this->ui->curTmpLabel_9->setText("");
                this->ui->tgTmpLabel_9->setText("");
                this->ui->elecLabel_9->setText("");
                this->ui->chargeLabel_9->setText("");
                this->ui->fanLabel_9->setText("");
                break;
             default:
                break;
            }
        }
    }
}

QString Server::fanToString(int f)
{
    if(f == 0)
    {
        return QString("");
    }
    else if(f == 1)
    {
        return QString("低");
    }
    else if(f == 2)
    {
        return QString("中");
    }
    else if(f == 3)
    {
        return QString("高");
    }
}

QString Server::stateToString(ClientState s)
{
    if(s == BOOT)
    {
        return QString("开机");
    }
    else if(s == CLOSE)
    {
        return QString("关机");
    }
    else if(s == AT_SERVICE)
    {
        return QString("服务中");
    }
    else if(s == SLEEP)
    {
        return QString("待机");
    }
    else if(s == WAIT)
    {
        return QString("等待中");
    }
}

