#include "server.h"
#include "ui_server.h"
#include <QtNetwork>
#include <QDebug>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include <QMessageBox>
#include <fstream>
#include "checkout.h"
#define T 2



//QTime current_time =QTime::currentTime();
QDateTime current_time = QDateTime::currentDateTime();

Server::Server(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Server)
{
    ui->setupUi(this);

    // 数据库初始化
    db = QSqlDatabase::addDatabase("QMYSQL");

    // 设置账号密码
    db.setHostName("127.0.0.1");
    db.setDatabaseName("air");
    db.setUserName("root");
    db.setPassword("123456");

    if(!db.open()){
        qDebug() << "operation error!";
        qDebug() << db.lastError();
        return;
    }

    //删除旧表
    QSqlQuery query(db);
    QString sql;
    bool success;
    sql = QString("drop table log");
    success = query.exec(sql);

    sql = QString("create table if not exists log \
                (room_id varchar(40) not null, \
                req_time datetime, \
                cur_time datetime,  \
                event varchar(10) not null,\
                cur_tp float,\
                target_tp float, \
                wind_speed int, \
                elec float, \
                charge float)");
    success = query.exec(sql);

    if(!success){
       qDebug() << "creating table fails!";
    }

    //读取初始化用数据用
    sql = QString("select * from parameters");
    query.exec(sql);
    query.next();
    ecost[2] = (double)query.value(0).toInt();
//    qDebug() << ecost[2];
    ecost[1] = (double)query.value(1).toInt();
//    qDebug() << ecost[1];
    ecost[0] = (double)query.value(2).toInt();
//    qDebug() << ecost[0];
    eprice = query.value(3).toDouble();
//    qDebug() << eprice;
    cgtp_pere = query.value(4).toDouble();
//    qDebug() << cgtp_pere;
    serve_mod = query.value(5).toInt();
//    qDebug() << serve_mod;
    t_high = (double)query.value(6).toDouble();
//    qDebug() << t_high;
    t_low = (double)query.value(7).toDouble();
//    qDebug() << t_low;
    MAXSERVE = query.value(8).toInt();
//    qDebug() << MAXSERVE;
    sec_per_minute = query.value(9).toInt();
//    qDebug() << sec_per_minute;
    timeslot = query.value(10).toInt();
//    qDebug() << timeslot;
    tp_range = (double)query.value(11).toInt();
//    qDebug() << tp_range;
    default_tg_tmp_high = default_tg_tmp_low = query.value(12).toDouble();
//    qDebug() << default_tg_tmp_high;
    default_fan = query.value(13).toInt();
//    qDebug() << default_fan;
    slot = 1;

    int n_cycle = (sec_per_minute * timeslot / slot);
    qDebug() << n_cycle;
    for(int i = 0; i < 3; i++)
        que[i] = Acque(n_cycle);

//    wait_num=0;
//    serve_num=0;
//    serve_mod=0; // 0制冷 1制热
//    ecost[0]=2;
//    ecost[1]=4;
//    ecost[2]=8;
//    eprice=0.02;
//    t_high=30.0;
//    t_low=16.0;
//    MAXSERVE=_MAXSERVE;
//    tp_range=2.0;
//    target_t=target_w=target_x=NULL;
//    cgtp_pere=0.03;

    tcpServer = new QTcpServer(this);
    if(!tcpServer->listen(QHostAddress::Any, 6666))
    {
        qDebug()<<tcpServer->errorString();
        close();
    }
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(connecting()));

//    send_back_timer = new QTimer();                          //新建定时回送计时器, 张尚之加
//    send_back_timer->start(1000);                            //定时回送事件，张尚之加
//    connect(send_back_timer, SIGNAL(timeout()), this, SLOT(refreshInfoWindow())); //定时回送事件，张尚之加

    cmp_log_timer = new QTimer();                          //新建定时回送计时器, 张尚之加
    cmp_log_timer->start(slot * 1000);                            //定时回送事件，张尚之加
    connect(cmp_log_timer, SIGNAL(timeout()), this, SLOT(cycleCompute())); //定时回送事件，张尚之加

//    send_back_timer = new QTimer();
//    send_back_timer->start(60000);
//    connect(send_back_timer, SIGNAL(timeout()), this, SLOT(cycleSendBack()));
//    connect(send_back_timer, SIGNAL(timeout()), this, SLOT(refreshInfoWindow()));

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

    //初始化
    for(int i = 0; i < 100; i ++)
        acdevice[i] = -1;


    this->ui->rp_date->setDateTime(QDateTime::currentDateTime());
    rpWindow = new checkout(this);
    this->rpWindow->setVisible(false);
    connect(this->rpWindow, SIGNAL(query(QString,QDateTime,QDateTime)),
            this, SLOT(answer_and_show(QString,QDateTime,QDateTime)));
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

void Server::cyclePrint()
{

//    for(int i=0;i<MAXSERVE;i++)
//    {
//        QVector<int> ttt=ac[i].getShareQ();
//        QString debuggg="";
//        debuggg="AC["+QString::number(i+1)+"] ";
//        for(QVector<int>::iterator it=ttt.begin();it!=ttt.end();it++){
//            debuggg+=room_info[*it].room_id+"@"+QString::number(room_info[*it].wind_speed)+" ";
//        }
//        if(ac[i].isOn)
//            qDebug()<<debuggg;
//    }
//    QString debuggg = "WAIT: ";
//    for(int i=0; i < room_info.length(); i++)
//    {
//        if(room_info[i].state == WAIT2)
//        {
//            debuggg += room_info[i].room_id + "@" + QString::number(room_info[i].wind_speed) + " ";
//        }
//    }
//    if(debuggg != "WAIT: ")
//        qDebug()<<debuggg;
//    return;
    for(int i=0;i<3;i++) que[i].disp();
    return;
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
    //qDebug()<<"timer";

    this->compute();
    this->cycleSendBack();
    this->refreshInfoWindow();

    this->cyclePrint();
}

void Server::send_data(QTcpSocket* tcpSocket, int mod, QString extra)
{
    int idx = this->search_idx(tcpSocket);  //张尚之加
    //qDebug() << room_info[idx].room_id;
    QString x;
    switch (mod) {
        case 0:
        // 0-被连接得到start请求，回应：  start_房间号_工作模式_工作温度范围_温度波动范围
            x = "start";
            x += "_" + extra;     //start_id是什么？
            x += "_" + QString::number(serve_mod);
//            x += "_" + QString::number(t_low,'f',2) + "-" + QString::number(t_high,'f',2);
//            x += "_" + QString::number(tp_range,'f',2);
            x += "_" + QString::number((int)t_low) + "-" + QString::number((int)t_high);
            x += "_" + QString::number((int)tp_range);
            break;
        case 1:
        // 1-服务过程中回应：            a_房间号_当前房间温度_累计计费_timenow_目标温度_风速_温度变化_单价_累计电量
            x = "a";
            x += "_" + (room_info[idx].room_id).toUtf8();
            x += "_" + QString("%1").arg(room_info[idx].cur_tp);
            x += "_" + QString("%1").arg(room_info[idx].charge);
//            x += "_" + QString::number(current_time.hour())+"/" +QString::number(current_time.minute())+"/"+QString::number(current_time.second());
            x += "_" + QString::number(current_time.time().hour())+\
                 "/" + QString::number(current_time.time().minute())+\
                 "/" + QString::number(current_time.time().second());
            x += "_" + QString("%1").arg(room_info[idx].target_tp);
            x += "_" + QString("%1").arg(room_info[idx].wind_speed);
            x += "_" + QString("%1").arg(room_info[idx].change_tp);
            x += "_" + QString("%1").arg(room_info[idx].price_3s);
            x += "_" + QString("%1").arg(room_info[idx].elec);
            break;
        case 2:
        // 2-关机：                    close_房间号
            x = "close";
            x += "_" + (room_info[idx].room_id).toUtf8() + "_" + extra;
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
            x += extra;           //extra 填写 序号与等待方式
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
            //qDebug() << room_id;
            room_id_to_idx[room_id] = idx;
            room_info[idx].room_id = room_id;
            room_info[idx].shedule_times = 0;
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
            room_info[idx].req_time = QDateTime::currentDateTime();
            room_info[idx].index = idx;
            room_info[idx].state=BOOT;

            write_log(room_info[idx].room_id, "request");      //写日志

            // 615
            addQ(idx);
            //qDebug()<<"add ok";

//            int req_state = this->put(idx, room_info[idx].wind_speed);
//            if(req_state == -1)
//            {
//                send_data(tcpSocket, 4, 0);
//                room_info[idx].state = WAIT;
//            }
//            else
//            {
//                room_info[idx].state = AT_SERVICE;
//            }
        }
        else if(res[0]=="c")//更改：c_房间号_当前温度_目标温度_风速
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

            int oldSpeed  = room_info[idx].wind_speed;
            if(res[4] == "#")
            {
                room_info[idx].wind_speed = this->default_fan;
            }
            else
            {
                room_info[idx].wind_speed = res[4].toInt();
            }
            room_info[idx].req_time = QDateTime::currentDateTime();
            room_info[idx].index = idx;
            write_log(room_info[idx].room_id, "change");      //写日志

//            if(room_info[idx].state==SLEEP) {
//                // 615
//                addQ(idx);

//                // this->put(idx,room_info[idx].wind_speed);
//            }
//            else{
//                // 615
//                // this->changeReq(idx, oldSpeed, room_info[idx].wind_speed);
//                changeQ(idx,oldSpeed);
//            }
            changeQ(idx,oldSpeed);
//            if(room_info[idx].state == WAIT1)
//            {
//                send_data(tcpSocket, 4, 0);
//            }
//            else if(room_info[idx].state == WAIT2)
//            {

//            }
        }
        else if(res[0]=="close")//关机：close_房间号
        {

            write_log(room_info[idx].room_id, "closed");      //写日志

            if(room_info[idx].state == AT_SERVICE)
            {
                // 615


//                for(int i = 0; i < MAXSERVE; i ++)
//                {
//                    if(ac[i].contains(idx))
//                    {
//                        ac[i].removeShare(idx);
//                        balanceAC();
//                        break;
//                    }
//                }
            }
            room_info[idx].state = CLOSE;
            // 615
            removeQ(idx);

            send_data(tcpSocket, 2, 0);
        }
    }
    cycleSendBack();
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

void Server::write_log(QString room_id, QString event){
    // 查找room_id对应的在room_info数组中的序号
    int index;
    for(int i = 0; i < room_info.length(); i++)
    {
        if(room_info[i].room_id == room_id){
            index = i;
            break;
        }
    }
    // 插入
    QSqlQuery query(db);
    QString sql = QString("INSERT INTO log(room_id, req_time, cur_time, event, cur_tp, target_tp, "
                          "wind_speed, elec, charge)");
    sql += QString(" Values('%1', '%2', '%3', '%4', '%5','%6', '%7', '%8', '%9')").arg(room_info[index].room_id)
            .arg(room_info[index].req_time.toString("yyyy-MM-dd hh:mm:ss"))
            .arg((QDateTime::currentDateTime()).toString("yyyy-MM-dd hh:mm:ss"))
            .arg(event)
            .arg(room_info[index].cur_tp )
            .arg(room_info[index].target_tp )
            .arg(room_info[index].wind_speed)
            .arg(room_info[index].elec)
            .arg(room_info[index].charge);
    qDebug() << sql;

    bool ok = query.exec(sql);
    if(!ok){
       qDebug() << "insert fails!";
       qDebug() << db.lastError();
    }
    else{
       qDebug() << "insert success!";
    }
}


QVector<dbrow> Server::query(QString room_id, QDateTime start, QDateTime end){
    QVector<dbrow> res;
    QSqlQuery query(db);
    QString sql = QString("select * from log "
                  "where room_id = '%1' and req_time >= '%2' and cur_time <= '%3'").arg(room_id)
                  .arg(start.toString("yyyy-MM-dd hh:mm:ss")).arg(end.toString("yyyy-MM-dd hh:mm:ss"));

    query.exec(sql);
    while(query.next()){
        dbrow tmp;
        tmp.room_id = room_id;
        tmp.req_time = query.value(1).toDateTime();
        tmp.cur_time = query.value(2).toDateTime();
        tmp.event = query.value(3).toString();
        tmp.cur_tp = query.value(4).toFloat();
        tmp.target_tp = query.value(5).toFloat();
        tmp.wind_speed = query.value(6).toInt();
        tmp.elec = query.value(7).toFloat();
        tmp.charge = query.value(8).toFloat();
        res.push_back(tmp);
    }

    return res;
}

rprow Server::get_rprow(QString room_id, QDateTime query_date)
{
    rprow r;
    QSqlQuery query(db);
    QString sql;
    //查询使用次数
    sql = QString("select count(*) from log "
                  "where room_id = '%1' and "
                  "to_days(req_time) = to_days('%2') and "
                  "event = 'closed'")
                  .arg(room_id)
                  .arg(query_date.toString("yyyy-MM-dd"));
    qDebug() << sql;
    query.exec(sql);
    query.next();
    r.times = query.value(0).toInt();
    qDebug() << "Use Times" << QString::number(r.times);

    //查询最常用目标温度与次数
    sql = QString("create view v as "
                  "select "
                  "target_tp as target_tp, "
                  "count(target_tp) as cnt "
                  "from ( "
                  "select room_id, target_tp, event "
                  "from log "
                  "where room_id = '%1' and "
                  "to_days(req_time) = to_days('%2') and "
                  "(event = 'request' or event = 'change')) as tmp "
                  "group by target_tp;")
            .arg(room_id)
            .arg(query_date.toString("yyyy-MM-dd"));
    query.exec(sql);

    sql = QString("select target_tp, cnt "
                  "from v "
                  "where cnt >= all(select cnt from v);");
    query.exec(sql);
    query.next();
    r.freq_tg_tp = query.value(0).toDouble();
    r.freq_t_times = query.value(1).toDouble();

    sql = QString("drop view v;");
    query.exec(sql);
    qDebug() << "Freq Temp: " + QString::number(r.freq_tg_tp);
    qDebug() << "Freq Temp times: " + QString::number(r.freq_t_times);

    //查询最常用目标风速与次数
    sql = QString("create view v as "
                  "select "
                  "wind_speed as wind_speed, "
                  "count(wind_speed) as cnt "
                  "from ( "
                  "select room_id, wind_speed, event "
                  "from log "
                  "where room_id = '%1' and "
                  "to_days(req_time) = to_days('%2') and "
                  "(event = 'request' or event = 'change')) as tmp "
                  "group by wind_speed;")
            .arg(room_id)
            .arg(query_date.toString("yyyy-MM-dd"));
    query.exec(sql);

    sql = QString("select wind_speed, cnt "
                  "from v "
                  "where cnt >= all(select cnt from v);");
    query.exec(sql);
    query.next();
    r.freq_wind_speed = query.value(0).toDouble();
    r.freq_ws_times = query.value(1).toDouble();
    sql = QString("drop view v;");
    query.exec(sql);

    qDebug() << "Freq Wind: " + QString::number(r.freq_wind_speed);
    qDebug() << "Freq Wind times: " + QString::number(r.freq_ws_times);

    //查询达到目标次温度的次数
    sql = QString("select count(*) from log "
                  "where room_id = '%1' and "
                  "to_days(req_time) = to_days('%2') and "
                  "event = 'finished'")
                  .arg(room_id)
                  .arg(query_date.toString("yyyy-MM-dd"));
    //qDebug() << sql;
    query.exec(sql);
    query.next();
    r.n_finished = query.value(0).toInt();
    qDebug() << "Finished Times " << QString::number(r.times);

    //查询达到目标次温度的次数
    sql = QString("select count(*) from log "
                  "where room_id = '%1' and "
                  "to_days(req_time) = to_days('%2') and "
                  "event = 'finished'")
                  .arg(room_id)
                  .arg(query_date.toString("yyyy-MM-dd"));
    qDebug() << sql;
    query.exec(sql);
    query.next();
    r.n_finished = query.value(0).toInt();
    qDebug() << "Finished Times " << QString::number(r.times);

    //查询消费总量
    sql = QString("select charge from log where room_id = '%1' and "
                  "req_time >= all(select req_time from log "
                  "where room_id = '%2') order by charge desc; ")
                  .arg(room_id)
                  .arg(room_id);
    qDebug() << sql;
    query.exec(sql);
    query.next();
    r.sum_charges = query.value(0).toInt();
    qDebug() << "Total charges: " << QString::number(r.sum_charges);
    return r;
}

void Server::compute()
{
    //qDebug()<<"start compute";
    double * temp_ecost;
    double Eprice;

    QVector<int> outofservice;

    temp_ecost=get_ecost();
    Eprice=get_e_price();

    // 615
    select();
    //qDebug()<<"select ok";

    for(int i = 0; i < MAXSERVE; i++)
    {
        // 615
        int idx=acdevice[i];
        if(idx != -1){
            int wind_rank=room_info[idx].wind_speed;
            room_info[idx].elec+=temp_ecost[wind_rank-1]*T;
            room_info[idx].charge+=temp_ecost[wind_rank-1]*T*Eprice;
            room_info[idx].cur_tp+=(serve_mod==0?-1:1)*(temp_ecost[wind_rank-1]*T)*cgtp_pere;

            if((serve_mod==0 && room_info[idx].cur_tp<=room_info[idx].target_tp)||
                    (serve_mod==1 && room_info[idx].cur_tp>=room_info[idx].target_tp))
            {
                send_data(tcpSocket_vec.at(idx), 1, "0");   //先发a
                send_data(tcpSocket_vec.at(idx),3,0);       //再发sleep
                qDebug()<<"sleep";
                //todo
                write_log(room_info[idx].room_id, "finished");      //写日志
                room_info[idx].state=SLEEP;
                outofservice.push_back(idx);
            }
        }   
    }
    updateQ();
    for(QVector<int>::iterator it=outofservice.begin();it!=outofservice.end();it++)
        removeQ(*it);
    //qDebug()<<"update ok";
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
double * Server::get_ecost()
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

// add 615
int Server::select(){   // 选择用户添加到acdevice中，改状态all

    // 改atservice
    int maxser=MAXSERVE;
    for(int i=2;i>=0;i--){
        int index=0;
        while(index<que[i].length()&&maxser>0){
            room* temp=que[i].at(index);
            temp->state=AT_SERVICE;
            maxser--;
            index++;
            acdevice[maxser]=temp->index;
        }
        if(maxser<=0) break;
    }
    // 找空调最低风
    int lowlevel=4; // init out of boundary!!!!!!!!!!!!!!!!!!!!!!!!!
    for(int i=0;i<MAXSERVE;i++){
        if(acdevice[i] != -1)
        {
            if(room_info[acdevice[i]].wind_speed<lowlevel)
                lowlevel=room_info[acdevice[i]].wind_speed;
        }
    }

    if(lowlevel!=4){
        for(int i=0;i<que[lowlevel-1].length();i++){
            if(que[lowlevel-1].at(i)->state==AT_SERVICE||que[lowlevel-1].at(i)->state==BOOT){
                int found=false;
                for(int j=0;j<MAXSERVE;j++){
                    if(que[lowlevel-1].at(i)->index==acdevice[j]){
                        found=true;
                        break;
                    }
                }
                if(!found){
                    que[lowlevel-1].at(i)->shedule_times++;
                    que[lowlevel-1].at(i)->state=WAIT2;
                    send_data(tcpSocket_vec[que[lowlevel-1].at(i)->index],4,"_"+QString::number(i)+"_2");
                    write_log(que[lowlevel-1].at(i)->room_id, "WAIT2");
                }
                else{
                    continue;
                }
            }
            else{
               que[lowlevel-1].at(i)->state=WAIT2;
               send_data(tcpSocket_vec[que[lowlevel-1].at(i)->index],4,"_"+QString::number(i)+"_2");
                write_log(que[lowlevel-1].at(i)->room_id, "WAIT2");
            }
        }
    }

    for(int i=0;i<lowlevel-1;i++){
        for(int j=0;j<que[i].length();j++){
            if(que[i].at(j)->state==WAIT2||que[i].at(j)->state==AT_SERVICE||que[i].at(j)->state==BOOT){
                que[i].at(j)->shedule_times++;
                que[i].at(j)->state=WAIT1;
                send_data(tcpSocket_vec[que[i].at(j)->index],4,"_0_1");
                write_log(que[lowlevel-1].at(i)->room_id, "WAIT1");
            }
        }
    }
//    QVector<int> outqueue;    // 被换出的队列
//    for(int i=0;i<3;i++){
//        for(int j=0;j<que[i].length();j++){
//            if(que[i].at(j)->state==AT_SERVICE||que[i].at(j)->state==BOOT){
//                bool found=false;
//                for(int k=0;k<MAXSERVE;k++){
//                    if(acdevice[k]==que[i].at(j)->index){
//                        found=true;
//                        break;
//                    }
//                }
//                if(!found){
//                    if(lowlevel==que[i].at(j)->wind_speed){
//                        que[i].at(j)->state=WAIT2;
//                    }
//                    else{
//                        qDebug()<<i;
//                        outqueue.push_back(i);
//                        break;
//                    }
//                }
//            }
//        }
//    }

//    for(int i=0;i<outqueue.size();i++){
//        int qidx=outqueue[i];
//        for(int j=0;j<que[qidx].length();j++){
//            que[qidx].at(j)->state=WAIT1;
//            send_data(tcpSocket_vec[que[qidx].at(j)->index],4,"_0_1");
//        }
//    }

//    // 设置same level轮转状态
//    if(lowlevel!=4){
//        for(int j=0;j<que[lowlevel-1].length();j++){
//            if(que[lowlevel-1].at(j)->state!=AT_SERVICE) {
//                que[lowlevel-1].at(j)->state=WAIT2;
//                qDebug()<<QString::number(lowlevel);
//                send_data(tcpSocket_vec[que[lowlevel-1].at(j)->index],4,"_"+QString::number(j)+"_2");
//            }
//        }
//    }




//    // 全部设置wait1状态，旧的atservice清空并发送消息
//    for(int i=0;i<3;i++){
//        for(int j=0;j<que[i].length();j++){
//            if(que[i].at(j)->state==AT_SERVICE||que[i].at(j)->state==BOOT){
//                bool found=false;
//                qDebug()<<"testidx: "<<que[i].at(j)->index;
//                for(int k=0;k<MAXSERVE;k++){
//                    qDebug()<<acdevice[k];
//                    if(acdevice[k]==que[i].at(j)->index){
//                        found=true;
//                        break;
//                    }
//                }
//                if(!found&&que[i].at(j)->index==lowlevel){
//                    send_data(tcpSocket_vec[que[i].at(j)->index],4,"_0_1");
//                    que[i].at(j)->state=WAIT1;
//                }
//            }
//            else{
//                que[i].at(j)->state=WAIT1;
//            }
//        }
//    }

//    // 设置same level轮转状态
//    if(lowlevel!=4){
//        for(int j=0;j<que[lowlevel-1].length();j++){
//            if(que[lowlevel-1].at(j)->state!=AT_SERVICE) {
//                que[lowlevel-1].at(j)->state=WAIT2;
//                qDebug()<<QString::number(lowlevel);
//                send_data(tcpSocket_vec[que[lowlevel-1].at(j)->index],4,"_"+QString::number(j)+"_2");
//            }
//        }
//    }
    return 0;
}

int Server::updateQ()
{
    int lowest=4;
    for(int i=0;i<MAXSERVE;i++){
        if(acdevice[i]!=-1&&room_info[acdevice[i]].wind_speed<lowest)
            lowest=room_info[acdevice[i]].wind_speed;
    }

    // 恢复状态，清空acdevice
    for(int i=0;i<MAXSERVE;i++) {
        //if(acdevice[i]!=-1)
            //room_info[acdevice[i]].state=WAIT1;
        acdevice[i]=-1;
    }

    for(int i=lowest-1;i<3;i++){
        if(!que[i].empty())
            que[i].innov();
    }
    return 0;
}

int Server::removeQ(int idx)
{
    QVector<room*>::iterator pos;
    int rank=room_info[idx].wind_speed-1;
    que[rank].remove(idx);
    return 0;
}

int Server::addQ(int idx)
{
    return que[room_info[idx].wind_speed-1].push(&room_info[idx]);
}

int Server::changeQ(int idx,int oldwind)
{
    // delete old
    for(int i=0;i<que[oldwind-1].length();i++){
        if(que[oldwind-1].at(i)->index==idx){
            que[oldwind-1].remove(idx);
            break;
        }
    }
    // add new
    addQ(idx);
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
        //qDebug()<<"idx: "<<idx<<endl;
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
            this->ui->kick_0->setDisabled(false);
            break;
        case 1:
            this->ui->stateLabel_1->setText(stateToString(curRoom->state));
            this->ui->curTmpLabel_1->setText(QString::number((int)curRoom->cur_tp));
            this->ui->tgTmpLabel_1->setText(QString::number((int)curRoom->target_tp));
            this->ui->elecLabel_1->setText(QString::number(curRoom->elec, 'f', 2));
            this->ui->chargeLabel_1->setText(QString::number(curRoom->charge, 'f', 2));
            this->ui->fanLabel_1->setText(fanToString(curRoom->wind_speed));
            this->ui->kick_1->setDisabled(false);
            break;
        case 2:
            this->ui->stateLabel_2->setText(stateToString(curRoom->state));
            this->ui->curTmpLabel_2->setText(QString::number((int)curRoom->cur_tp));
            this->ui->tgTmpLabel_2->setText(QString::number((int)curRoom->target_tp));
            this->ui->elecLabel_2->setText(QString::number(curRoom->elec, 'f', 2));
            this->ui->chargeLabel_2->setText(QString::number(curRoom->charge, 'f', 2));
            this->ui->fanLabel_2->setText(fanToString(curRoom->wind_speed));
            this->ui->kick_2->setDisabled(false);
            break;
        case 3:
            this->ui->stateLabel_3->setText(stateToString(curRoom->state));
            this->ui->curTmpLabel_3->setText(QString::number((int)curRoom->cur_tp));
            this->ui->tgTmpLabel_3->setText(QString::number((int)curRoom->target_tp));
            this->ui->elecLabel_3->setText(QString::number(curRoom->elec, 'f', 2));
            this->ui->chargeLabel_3->setText(QString::number(curRoom->charge, 'f', 2));
            this->ui->fanLabel_3->setText(fanToString(curRoom->wind_speed));
            this->ui->kick_3->setDisabled(false);
            break;
        case 4:
            this->ui->stateLabel_4->setText(stateToString(curRoom->state));
            this->ui->curTmpLabel_4->setText(QString::number((int)curRoom->cur_tp));
            this->ui->tgTmpLabel_4->setText(QString::number((int)curRoom->target_tp));
            this->ui->elecLabel_4->setText(QString::number(curRoom->elec, 'f', 2));
            this->ui->chargeLabel_4->setText(QString::number(curRoom->charge, 'f', 2));
            this->ui->fanLabel_4->setText(fanToString(curRoom->wind_speed));
            this->ui->kick_4->setDisabled(false);
            break;
        case 5:
            this->ui->stateLabel_5->setText(stateToString(curRoom->state));
            this->ui->curTmpLabel_5->setText(QString::number((int)curRoom->cur_tp));
            this->ui->tgTmpLabel_5->setText(QString::number((int)curRoom->target_tp));
            this->ui->elecLabel_5->setText(QString::number(curRoom->elec, 'f', 2));
            this->ui->chargeLabel_5->setText(QString::number(curRoom->charge, 'f', 2));
            this->ui->fanLabel_5->setText(fanToString(curRoom->wind_speed));
            this->ui->kick_5->setDisabled(false);
            break;
        case 6:
            this->ui->stateLabel_6->setText(stateToString(curRoom->state));
            this->ui->curTmpLabel_6->setText(QString::number((int)curRoom->cur_tp));
            this->ui->tgTmpLabel_6->setText(QString::number((int)curRoom->target_tp));
            this->ui->elecLabel_6->setText(QString::number(curRoom->elec, 'f', 2));
            this->ui->chargeLabel_6->setText(QString::number(curRoom->charge, 'f', 2));
            this->ui->fanLabel_6->setText(fanToString(curRoom->wind_speed));
            this->ui->kick_6->setDisabled(false);
            break;
        case 7:
            this->ui->stateLabel_7->setText(stateToString(curRoom->state));
            this->ui->curTmpLabel_7->setText(QString::number((int)curRoom->cur_tp));
            this->ui->tgTmpLabel_7->setText(QString::number((int)curRoom->target_tp));
            this->ui->elecLabel_7->setText(QString::number(curRoom->elec, 'f', 2));
            this->ui->chargeLabel_7->setText(QString::number(curRoom->charge, 'f', 2));
            this->ui->fanLabel_7->setText(fanToString(curRoom->wind_speed));
            this->ui->kick_7->setDisabled(false);
            break;
        case 8:
            this->ui->stateLabel_8->setText(stateToString(curRoom->state));
            this->ui->curTmpLabel_8->setText(QString::number((int)curRoom->cur_tp));
            this->ui->tgTmpLabel_8->setText(QString::number((int)curRoom->target_tp));
            this->ui->elecLabel_8->setText(QString::number(curRoom->elec, 'f', 2));
            this->ui->chargeLabel_8->setText(QString::number(curRoom->charge, 'f', 2));
            this->ui->fanLabel_8->setText(fanToString(curRoom->wind_speed));
            this->ui->kick_8->setDisabled(false);
            break;
        case 9:
            this->ui->stateLabel_9->setText(stateToString(curRoom->state));
            this->ui->curTmpLabel_9->setText(QString::number((int)curRoom->cur_tp));
            this->ui->tgTmpLabel_9->setText(QString::number((int)curRoom->target_tp));
            this->ui->elecLabel_9->setText(QString::number(curRoom->elec, 'f', 2));
            this->ui->chargeLabel_9->setText(QString::number(curRoom->charge, 'f', 2));
            this->ui->fanLabel_9->setText(fanToString(curRoom->wind_speed));
            this->ui->kick_9->setDisabled(false);
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
                this->ui->kick_0->setDisabled(true);
                break;
            case 1:
                this->ui->stateLabel_1->setText("未连接");
                this->ui->curTmpLabel_1->setText("");
                this->ui->tgTmpLabel_1->setText("");
                this->ui->elecLabel_1->setText("");
                this->ui->chargeLabel_1->setText("");
                this->ui->fanLabel_1->setText("");
                this->ui->kick_1->setDisabled(true);
                break;
            case 2:
                this->ui->stateLabel_2->setText("未连接");
                this->ui->curTmpLabel_2->setText("");
                this->ui->tgTmpLabel_2->setText("");
                this->ui->elecLabel_2->setText("");
                this->ui->chargeLabel_2->setText("");
                this->ui->fanLabel_2->setText("");
                this->ui->kick_2->setDisabled(true);
                break;
            case 3:
                this->ui->stateLabel_3->setText("未连接");
                this->ui->curTmpLabel_3->setText("");
                this->ui->tgTmpLabel_3->setText("");
                this->ui->elecLabel_3->setText("");
                this->ui->chargeLabel_3->setText("");
                this->ui->fanLabel_3->setText("");
                this->ui->kick_3->setDisabled(true);
                break;
            case 4:
                this->ui->stateLabel_4->setText("未连接");
                this->ui->curTmpLabel_4->setText("");
                this->ui->tgTmpLabel_4->setText("");
                this->ui->elecLabel_4->setText("");
                this->ui->chargeLabel_4->setText("");
                this->ui->fanLabel_4->setText("");
                this->ui->kick_4->setDisabled(true);
                break;
            case 5:
                this->ui->stateLabel_5->setText("未连接");
                this->ui->curTmpLabel_5->setText("");
                this->ui->tgTmpLabel_5->setText("");
                this->ui->elecLabel_5->setText("");
                this->ui->chargeLabel_5->setText("");
                this->ui->fanLabel_5->setText("");
                this->ui->kick_5->setDisabled(true);
                break;
            case 6:
                this->ui->stateLabel_6->setText("未连接");
                this->ui->curTmpLabel_6->setText("");
                this->ui->tgTmpLabel_6->setText("");
                this->ui->elecLabel_6->setText("");
                this->ui->chargeLabel_6->setText("");
                this->ui->fanLabel_6->setText("");
                this->ui->kick_6->setDisabled(true);
                break;
            case 7:
                this->ui->stateLabel_7->setText("未连接");
                this->ui->curTmpLabel_7->setText("");
                this->ui->tgTmpLabel_7->setText("");
                this->ui->elecLabel_7->setText("");
                this->ui->chargeLabel_7->setText("");
                this->ui->fanLabel_7->setText("");
                this->ui->kick_7->setDisabled(true);
                break;
           case 8:
                this->ui->stateLabel_8->setText("未连接");
                this->ui->curTmpLabel_8->setText("");
                this->ui->tgTmpLabel_8->setText("");
                this->ui->elecLabel_8->setText("");
                this->ui->chargeLabel_8->setText("");
                this->ui->fanLabel_8->setText("");
                this->ui->kick_8->setDisabled(true);
                break;
            case 9:
                this->ui->stateLabel_9->setText("未连接");
                this->ui->curTmpLabel_9->setText("");
                this->ui->tgTmpLabel_9->setText("");
                this->ui->elecLabel_9->setText("");
                this->ui->chargeLabel_9->setText("");
                this->ui->fanLabel_9->setText("");
                this->ui->kick_9->setDisabled(true);
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
    else if(s == WAIT1)
    {
        return QString("等待中");
    }
    else if(s == WAIT2)
    {
        return QString("轮转中");
    }

}

void Server::on_pushButton_clicked()
{
    QString room_id = this->ui->rp_id->currentText();
    QDateTime rep_date = this->ui->rp_date->dateTime();
    rprow rp = this->get_rprow(room_id, rep_date);
    int idx = -1;
    for(int i = room_info.length() - 1; i >= 0; i--)
    {
        if(room_info[i].room_id == room_id)
        {
            idx = i;
            break;
        }
    }
    int n_schedules = (idx == -1? 0: room_info[idx].shedule_times);
    QString display = QString("房间: %1 \n"
                              "使用次数: %2 \n"
                              "最常用目标温度: %3（%4次）\n"
                              "最常用风速: %5 (%6次)\n"
                              "完成次数: %7 \n"
                              "调度次数： %8 \n"
                              "总费用: %9 \n")
                        .arg(room_id)
                        .arg(QString::number(rp.times))
                        .arg(QString::number(rp.freq_tg_tp)).arg(QString::number(rp.freq_t_times))
                        .arg(this->fanToString(rp.freq_wind_speed)).arg(QString::number(rp.freq_ws_times))
                        .arg(QString::number(rp.n_finished))
                        .arg(QString::number(n_schedules))
                        .arg(QString::number(rp.sum_charges, 'g', 2));
    int ret = QMessageBox::information(this, "报表", display);
}

// 615

room *Acque::top()
{
    return list.front();
}

int Acque::push(room *r)
{
    if(r==NULL) return -1;
    list.push_back(r);
    return 0;
}

int Acque::pop()
{
    QVector<room*>::iterator temp=list.begin();
    list.erase(list.begin());
    return 0;
}

room *Acque::get(int room_idx)
{
    room* res=NULL;
    for(QVector<room*>::iterator it=list.begin();it!=list.end();it++){
        if((*it)->index==room_idx){
            res=*it;
            break;
        }
    }
    return res;
}

bool Acque::empty()
{
    return list.empty();
}

int Acque::length()
{
    return list.size();
}

int Acque::cycle()
{
    if(list.empty()){
        return -1;
    }
    room * temp=list.front();
    QVector<room*>::iterator t=list.begin();
    list.erase(t);
    list.push_back(temp);
    return 0;
}

room *Acque::at(int idx)
{
    return list.at(idx);
}

int Acque::getcurslot()
{
    return cnt;
}

int Acque::getslot()
{
    return timeslot;
}

int Acque::setslot(int t)
{
    timeslot=t;
    cnt=t;
    return 0;
}

int Acque::innov()
{
    cnt--;
    if(cnt==0){
        cnt=timeslot;
        cycle();
    }
    return 0;
}

int Acque::remove(int idx)
{
    int tok=0;
    QVector<room*>::iterator pos;
    if(list.empty()) return 0;
    for(QVector<room* >::iterator it=list.begin();it!=list.end();it++){
        if((*it)->index==idx){
            pos=it;
            break;
        }
    }

    list.erase(pos);
    return 0;
}


void Server::checkout_and_reset(QString room_id)
{
    for(int i = room_info.length() - 1; i >= 0; i--)
    {
        if(room_info[i].room_id == room_id)     //清空
        {
            this->room_info[i].charge = 0.;
            this->room_info[i].elec = 0.;
            this->target_t = 0;
            this->room_info[i].state = CLOSE;

            removeQ(i);

            send_data(tcpSocket_vec[i], 2, "1");  //退房
            this->write_log(room_id, "checkout");
        }
    }
}


//退房
void Server::on_kick_0_clicked()
{
    checkout_and_reset("306C");
}

void Server::on_kick_1_clicked()
{
    checkout_and_reset("306D");
}

void Server::on_kick_2_clicked()
{
    checkout_and_reset("307C");
}

void Server::on_kick_3_clicked()
{
    checkout_and_reset("307D");
}

void Server::on_kick_4_clicked()
{
    checkout_and_reset("308C");
}

void Server::on_kick_5_clicked()
{
    checkout_and_reset("308D");
}

void Server::on_kick_6_clicked()
{
    checkout_and_reset("309C");
}

void Server::on_kick_7_clicked()
{
    checkout_and_reset("309D");
}

void Server::on_kick_8_clicked()
{
    checkout_and_reset("310C");
}

void Server::on_kick_9_clicked()
{
    checkout_and_reset("310D");
}

void Server::on_pushButton_2_clicked()
{
    this->rpWindow->clear();
    this->rpWindow->show();
}

void Server::answer_and_show(QString room_id, QDateTime s, QDateTime e)
{
    QVector<dbrow> answer = query(room_id, s, e);
    this->rpWindow->load_answer(answer);
}

//void Server::clear_window(QString room_id)
//{
//    int i = this->idToIdx[room_id];
//    switch(i)
//    {
//    case 0:
//        this->ui->stateLabel_0->setText("未连接");
//        this->ui->curTmpLabel_0->setText("");
//        this->ui->tgTmpLabel_0->setText("");
//        this->ui->elecLabel_0->setText("");
//        this->ui->chargeLabel_0->setText("");
//        this->ui->fanLabel_0->setText("");
//        this->ui->kick_0->setDisabled(true);
//        break;
//    case 1:
//        this->ui->stateLabel_1->setText("未连接");
//        this->ui->curTmpLabel_1->setText("");
//        this->ui->tgTmpLabel_1->setText("");
//        this->ui->elecLabel_1->setText("");
//        this->ui->chargeLabel_1->setText("");
//        this->ui->fanLabel_1->setText("");
//        this->ui->kick_1->setDisabled(true);
//        break;
//    case 2:
//        this->ui->stateLabel_2->setText("未连接");
//        this->ui->curTmpLabel_2->setText("");
//        this->ui->tgTmpLabel_2->setText("");
//        this->ui->elecLabel_2->setText("");
//        this->ui->chargeLabel_2->setText("");
//        this->ui->fanLabel_2->setText("");
//        this->ui->kick_2->setDisabled(true);
//        break;
//    case 3:
//        this->ui->stateLabel_3->setText("未连接");
//        this->ui->curTmpLabel_3->setText("");
//        this->ui->tgTmpLabel_3->setText("");
//        this->ui->elecLabel_3->setText("");
//        this->ui->chargeLabel_3->setText("");
//        this->ui->fanLabel_3->setText("");
//        this->ui->kick_3->setDisabled(true);
//        break;
//    case 4:
//        this->ui->stateLabel_4->setText("未连接");
//        this->ui->curTmpLabel_4->setText("");
//        this->ui->tgTmpLabel_4->setText("");
//        this->ui->elecLabel_4->setText("");
//        this->ui->chargeLabel_4->setText("");
//        this->ui->fanLabel_4->setText("");
//        this->ui->kick_4->setDisabled(true);
//        break;
//    case 5:
//        this->ui->curTmpLabel_5->setText("");
//        this->ui->tgTmpLabel_5->setText("");
//        this->ui->elecLabel_5->setText("");
//        this->ui->chargeLabel_5->setText("");
//        this->ui->fanLabel_5->setText("");
//        this->ui->kick_5->setDisabled(true);
//        break;
//    case 6:
//        this->ui->curTmpLabel_6->setText("");
//        this->ui->tgTmpLabel_6->setText("");
//        this->ui->elecLabel_6->setText("");
//        this->ui->chargeLabel_6->setText("");
//        this->ui->fanLabel_6->setText("");
//        this->ui->kick_6->setDisabled(true);
//        break;
//    case 7:
//        this->ui->curTmpLabel_7->setText("");
//        this->ui->tgTmpLabel_7->setText("");
//        this->ui->elecLabel_7->setText("");
//        this->ui->chargeLabel_7->setText("");
//        this->ui->fanLabel_7->setText("");
//        this->ui->kick_7->setDisabled(true);
//        break;
//   case 8:
//        this->ui->curTmpLabel_8->setText("");
//        this->ui->tgTmpLabel_8->setText("");
//        this->ui->elecLabel_8->setText("");
//        this->ui->chargeLabel_8->setText("");
//        this->ui->fanLabel_8->setText("");
//        this->ui->kick_8->setDisabled(true);
//        break;
//    case 9:
//        this->ui->curTmpLabel_9->setText("");
//        this->ui->tgTmpLabel_9->setText("");
//        this->ui->elecLabel_9->setText("");
//        this->ui->chargeLabel_9->setText("");
//        this->ui->fanLabel_9->setText("");
//        this->ui->kick_9->setDisabled(true);
//        break;
//     default:
//        break;
//    }
//}

int Acque::disp()
{
//    QString res="";
//    if(!list.empty()) res+=(QString::number(list[0]->wind_speed)+": ");
//    for(int i=0;i<list.size();i++){
//        res+=(QString::number(list.at(i)->index)+"-");
//    }
//    if(res!="") qDebug() << res;
//    return 0;
}
