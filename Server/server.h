#ifndef SERVER_H
#define SERVER_H

#include <QDialog>
#include <QAbstractSocket>
#include <time.h>
#include <vector>
#include <fstream>
#include <QVector>
#include <QMap>
#include <queue>

namespace Ui {
class Server;
}

class QTcpSocket;

enum ClientState
{
    BOOT,          //启动
    CLOSE,         //关机
    AT_SERVICE,    //正在被服务
    SLEEP,         //服务完毕
    WAIT           //等待

};

typedef struct Room{

    QString room_id="";            //房间号
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
    float  price_3s;        //3s内产生的费用（我也不知道为什么设这个傻逼值）

    QTcpSocket* tcpSocket;   // 通信用socket

    QString req_time;       //最近请求时间

} room;

class QTcpServer;

class AirCond{
private:
    QVector<int> shareQ;    // store index of room in server room list at respective AC
    int tSlot;
    int current;    // local index
    int timeCnt;    // round robin current service count

public:
    AirCond(int slot);
    bool isRoundRobin;
    bool isOn;
    int getNext();
    int addShare(int id);
    int removeShare(int id);        // remove by id
    int removeShareIdx(int idx);    // remove by index return id
    int getNowServicing(); // global index
    int getWillServicing();         // return id
    bool contains(int id);
};


class Server : public QDialog
{
    Q_OBJECT

public:
    explicit Server(QWidget *parent = 0);
    ~Server();
    void write_log();    //接受compute()的数据记入数据库
    void compute();      //计算各房间温度、耗电量、费用并返回。
    //check_status(); //
    void findtarget(QString roomid);
//    void init_room(int i);
    void init_room(QTcpSocket* socket);
    float get_e_price();
    int * get_ecost();
    QVector<room>* get_room_arr();


private:
    Ui::Server *ui;

    //room    room_info[5];  //房间信息数组，3s刷新  一次  张尚之改， 5->10;
    QVector<room> room_info;      //张尚之加
    int     wait_num;
    int     serve_num;
    int     timeSlot;

    QTcpServer* tcpServer;
//    QTcpSocket* tcpSocket;
    QVector<QTcpSocket*> tcpSocket_vec;
    QMap<QString, int> room_id_to_idx;

    QString message;
    quint16 blockSize;

    int     ecost[3];   //    三种风速下的单位时间耗电量（度/分钟）
    float   eprice;     //    每度电的价格（元/度）（用来计算三种风速的价格）
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
    //get_list();     //读取详单

    QTimer* send_back_timer;     //用于定时回送数据 张尚之添加
    QTimer* cmp_log_timer;     //用于定时计算与写回数据 张尚之添加
    int search_idx(QTcpSocket* tcpSocket);

    QVector<AirCond> ac;        // AC entities

    std::priority_queue<room> request;
    int addReq(int room_id,int windspeed);  // return 0-OK,-1-DELAY
    int changeReq(int room_id,int oldSpeed,int newSpeed);






private slots:
    void recieve_request();        //监听端口，处理接收到的请求
//    void send_data(int mod, QString start_id);       //发送数据
    void send_data(QTcpSocket* tcpSocket, int mod, QString start_id);       //发送数据, 张尚之改
    void displayError(QAbstractSocket::SocketError);
    void connecting();


    void cycleSendBack();        //周期性回送消息 张尚之添加
    void cycleCompute();         //周期性计算数据 张尚之加
};

#endif // SERVER_H
