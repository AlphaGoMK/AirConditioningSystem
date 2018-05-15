#ifndef CLIENT_H
#define CLIENT_H

#include <QWidget>

namespace Ui {
class Client;
}

enum ClientState
{
    BOOT,          //启动
    CLOSE,         //关机
    AT_SERVICE,    //正在被服务
    SLEEP,         //服务完毕
    WAIT           //等待
};


class Client : public QWidget
{
    Q_OBJECT

public:
    explicit Client(QWidget *parent = 0);
    ~Client();
    Client(QWidget *parent = 0, int room_id);
private:
    Ui::Client *ui;

    int room_id;            //房间号
    double cur_tp;          //当前数据
    double t_high;          //最高温度
    double t_low;           //最低温度
    double threshold;       //温变阈值

    ClientState state;      //状态
    float target_tp;        //目标温度
    float wind_speed;       //风速

    float elec;             //耗电量
    float charge;           //费用

    bool recv_reg();     //接收服务端start信令，初始化Client
};

#endif // CLIENT_H
