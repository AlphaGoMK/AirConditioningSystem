#ifndef SERVER_H
#define SERVER_H

#include <QDialog>
#include <QAbstractSocket>
#include <time.h>
#include <QTime>
#include <QDateTime>
#include <vector>
#include <fstream>
#include <QVector>
#include <QMap>
#include <queue>
#include <QSqlError>
#include <Qtsql/QSqlDatabase>
#include <Qtsql/QSqlQuery>
#include <QSqlRecord>
#include <QtSql>
#include "aircond.h"
#include <QStack>

#define DEAFULTTIMESLOTS 3
#define _MAXSERVE 2

namespace Ui {
class Server;
}

class QTcpSocket;
class checkout;

enum ClientState
{
    BOOT,          //启动
    CLOSE,         //关机
    AT_SERVICE,    //正在被服务
    SLEEP,         //服务完毕
    WAIT,
    WAIT1,         //优先级等待
    WAIT2          //轮转等待
};

typedef struct Room{

    QString room_id="";     //房间号
    double t_high;          //最高温度
    double t_low;           //最低温度
    double threshold;       //温变阈值
    ClientState state;      //枚举状态
    float target_tp;        //目标温度
    int wind_speed;         //风速（为整数：1-低速；2-中速；3-高速）
    QString start_time;     //开机时间
    QString end_time;       //关机时间

    float elec;             //累计耗电量
    float charge;           //累计费用   
    double cur_tp;          //当前数据
    float change_tp;        //3s温度变化（有正负）
    float  price_3s;        //3s内产生的费用（我也不知道为什么设这个傻逼值
    int open_times;
    int shedule_times;      //调度次数

    QTcpSocket* tcpSocket;   // 通信用socket

    // ADD
//    QTime req_time;       //最近请求时间
    QDateTime req_time;
    int index;              //在room_info中的序号

} room;

struct dbrow {
    QString room_id;
    QDateTime req_time;
    QDateTime cur_time;
    QString event;
    float cur_tp;
    float target_tp;
    int wind_speed;
    float elec;
    float charge;
};

struct rprow
{
    QString room_id;     //房间号
    int times;           //使用次数
    double freq_tg_tp;   //最常用目标温度
    int freq_t_times;    //最常用目标温度使用次数
    int freq_wind_speed; //最常用风速
    int freq_ws_times;   //最常用风速使用次数
    int n_finished;      //完成次数
    double sum_charges;  //总费用
};


struct compare {
    bool operator () (const room* r1, const room* r2) const {
        if(r1->wind_speed==r2->wind_speed){
            return r1->req_time>r2->req_time;
        }
        else return r1->wind_speed<r2->wind_speed;
    }
};


// 615
class Acque{
private:
    int cnt;
    int timeslot;
    QVector<room *> list;
public:
    Acque(int ts){
        cnt=ts;
        timeslot=ts;
        list.empty();
    }
    Acque()
    {
        cnt=DEAFULTTIMESLOTS;
        timeslot=DEAFULTTIMESLOTS;
        list.empty();
    }
    room* top();
    int push(room* r);
    int pop();
    room* get(int room_idx);
    bool empty();
    int length();
    int cycle();
    room* at(int idx);
    int getcurslot();
    int getslot();
    int setslot(int t);
    int innov();
    int remove(int idx);
    int disp();
};


class QTcpServer;

class Server : public QDialog
{
    Q_OBJECT

public:
    explicit Server(QWidget *parent = 0);
    ~Server();
    void write_log(QString room_id, QString event);
    void compute();      //计算各房间温度、耗电量、费用并返回。
    void findtarget(QString roomid);
    void init_room(QTcpSocket* socket);
    float get_e_price();
    double * get_ecost();
    QVector<room>* get_room_arr();

    //数据库相关
    QVector<dbrow> query(QString room_id, QDateTime start, QDateTime end);
    QVector<rprow> get_rp(QString room_id, QDateTime query_date);
    rprow get_rprow(QString room_id, QDateTime query_date);


private:
    Ui::Server *ui;

    //room    room_info[5];  //房间信息数组，3s刷新  一次  张尚之改， 5->10;
    QVector<room> room_info;      //张尚之加
    int     wait_num;
    int     serve_num;
    int     timeSlot;

    int     default_tg_tmp_low = 26;
    int     default_tg_tmp_high = 30;
    int     default_fan = 3;

    QTcpServer* tcpServer;
//    QTcpSocket* tcpSocket;
    QVector<QTcpSocket*> tcpSocket_vec;
    QMap<QString, int> room_id_to_idx;

    QString message;
    quint16 blockSize;

    double  ecost[3];   //    三种风速下的单位时间耗电量（度/分钟）
    float   eprice;     //    每度电的价格（元/度）（用来计算三种风速的价格）
    int     slot;       //    系统多少秒计算一次温度
    int     sec_per_minute;    //每秒对应的分钟数
    int     timeslot;          //轮转时间片大小
    float   cgtp_pere;  //    单位电量温度下降幅度（摄氏度/度）（用来计算三种风速的温度下降幅度）
    int     serve_mod;  //    空调系统工作模式（0-制冷、1-制热）
    double  t_high;          //最高温度 空调的工作温度范围
    double  t_low;           //最低温度 空调的工作温度范围
    int     MAXSERVE;   //    同时能够服务的空调数（整数）
    int     vitual_time;//    每分钟系统时间长度（秒）（现实一分钟对应于系统的秒数，用来决定在服务过程中每隔多长时间进行一次通话，以及各种时间触发器的时间设定）
    int     round_time; //    空调队列轮转时间单位（分，需要执行的时候转换为系统秒）
    float   tp_range;   //    温度波动范围（度）
    room*   target_x;
    room*   target_t;
    room*   target_w;

    // database
    QSqlDatabase db;

    QTimer* send_back_timer;     //用于定时回送数据 张尚之添加
    QTimer* cmp_log_timer;     //用于定时计算与写回数据 张尚之添加
    int search_idx(QTcpSocket* tcpSocket);

    // 615

    int acdevice[100];    // 空调机
    Acque que[3];       // 空调等待队列
    int select();
    int updateQ();
    int changeQ(int idx,int oldwind);// 已经修改过风速
    int removeQ(int idx);// idx, index of room_info
    int addQ(int idx);

    QMap<QString, int> idToIdx;

    void checkout_and_reset(QString room_id);        //退房
    checkout *rpWindow;
    void clear_window(QString room_id);

private slots:
    void recieve_request();        //监听端口，处理接收到的请求
    void send_data(QTcpSocket* tcpSocket, int mod, QString extra);       //发送数据, 张尚之改
    void displayError(QAbstractSocket::SocketError);
    void connecting();

    void cyclePrint();

    void cycleSendBack();        //周期性回送消息 张尚之添加
    void cycleCompute();         //周期性计算数据 张尚之加
    void refreshInfoWindow();    //周期性刷新窗口数据
    QString stateToString(ClientState s);
    QString fanToString(int f);
    void on_pushButton_clicked();
    void on_kick_0_clicked();
    void on_kick_1_clicked();
    void on_kick_2_clicked();
    void on_kick_3_clicked();
    void on_kick_4_clicked();
    void on_kick_5_clicked();
    void on_kick_6_clicked();
    void on_kick_7_clicked();
    void on_kick_8_clicked();
    void on_kick_9_clicked();
    void on_pushButton_2_clicked();
    void answer_and_show(QString, QDateTime, QDateTime);
};

#endif // SERVER_H
