#ifndef AIRCOND_H
#define AIRCOND_H

#include "server.h"
#include "ui_server.h"
#include <QtNetwork>
#include <QDebug>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include <fstream>

class AirCond{
private:
    QVector<int> shareQ;    // room_info中的序号
    int tSlot;
    int current;    // local index
    int timeCnt;    // round robin current service count

public:
    AirCond(int slot);
    AirCond();
    bool isRoundRobin;
    bool isOn;
    int getNext();
    int addShare(int id);
    int removeShare(int id);        // remove by id(id: index in room_info. idx: index in shareQ
    int removeShareIdx(int idx);    // remove by index return id
    int getNowServicing();          // index in room_info
    int getWillServicing();         // return id
    int popback();
    bool contains(int id);

    QVector<int> getShareQ();
    int clearall(){
        shareQ.clear();
        return 0;
    }
};

#endif // AIRCOND_H
