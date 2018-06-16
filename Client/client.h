#ifndef CLIENT_H
#define CLIENT_H

#include <QWidget>
#include <winsock2.h>
#include <QtNetwork>
#include <stdio.h>
#include <windows.h>
#include <QString>
#include <qtimer.h>
#include "initwindow.h"
#include <QSqlError>
#include <Qtsql/QSqlDatabase>
#include <Qtsql/QSqlQuery>
#include <QSqlRecord>

#pragma comment(lib,"ws2_32.lib")

namespace Ui {
class Client;
}

enum ClientState
{
    BOOT,          //启动
    CLOSE,         //关机
    AT_SERVICE,    //正在被服务
    SLEEP,         //服务完毕
    WAIT,           //等待
    WAIT1,         //等待
    WAIT2          //轮转
};

enum WorkMode
{
    COLD,
    HOT
};

class Client : public QWidget
{
    Q_OBJECT

public:
    explicit Client(QWidget *parent = 0);
    ~Client();
    Client(int room_id, QWidget *parent = 0);
    InitWindow *initWindow;
signals:
    void sigModStatus();

private slots:
    void on_pushButton_7_clicked();
    void updateStatus();
    void check_inited();
    void parse_data();
    void fade(); //回温

    void on_pushButton_5_clicked();


    void on_pushButton_6_clicked();
//    void on_pushButton_clicked();


    void on_pushButton_2_clicked();
    void set_env_tp(int env_tp);
    void on_up_clicked();


private:
    Ui::Client *ui;

    bool inited;
    QString room_id;            //房间号
    double cur_tp;          //当前数据
    double t_high;          //最高温度
    double t_low;           //最低温度
    double threshold;       //温变阈值
    double env_tp;          //环境温度，回温的上限
    WorkMode mode;          //工作模式

    double fade_rate;

    ClientState state;      //状态
    int target_tp;        //目标温度
    int wind_speed;       //风速 123低中高

    float elec;             //耗电量
    float charge;           //费用

    SOCKET sock;            // written by mk
    struct sockaddr_in serverAddress; // written by mk

    QTcpSocket * tcp_socket;

    QTimer* fade_timer;

    QSqlDatabase db;

    bool recv_reg();     //接收服务端start信令，初始化Client
    int send_to_server(QString msg);
    int init_connect();
    int init();
    int close_connect();
    void refresh_screen(); //刷新显示
    void reset();   //退房后重新设置


};
#endif // CLIENT_H
