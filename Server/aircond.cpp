#include "aircond.h"

AirCond::AirCond(int slot){
    tSlot=slot;
    isOn=false;
    isRoundRobin=false;
    timeCnt=0;
    current=-1;
}

AirCond::AirCond()
{
    tSlot=1;
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
            break;
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

int AirCond::AirCond::popback()
{
    int res=shareQ.back();
    shareQ.pop_back();
    return res;
}

bool AirCond::contains(int id){
    for(QVector<int>::iterator it=shareQ.begin();it!=shareQ.end();it++){
        if(*it==id){
            return true;
        }
    }
    return false;
}

QVector<int> AirCond::getShareQ()
{
    return shareQ;
}
