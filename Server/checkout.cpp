#include "checkout.h"
#include "server.h"
#include "ui_checkout.h"

checkout::checkout(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::checkout)
{
    ui->setupUi(this);

//    ui->starttime = new  QDateTimeEdit(QDateTime::currentDateTime());
//    ui->endtime = new  QDateTimeEdit(QDateTime::currentDateTime());

    ui->tableWidget->setColumnCount(9); //设置列数
    ui->tableWidget->horizontalHeader()->setDefaultSectionSize(100);
    QStringList header;
    header<<tr("房间号")<<tr("开始时间")<<tr("结束时间")<<tr("事件")<<tr("当前温度")<<("目标温度")<<("风速")<<("耗电量")<<("费用");
    ui->tableWidget->setHorizontalHeaderLabels(header);

    ui->tableWidget->horizontalHeader()->setStretchLastSection(false);
    ui->tableWidget->setFrameShape(QFrame::NoFrame); //设置无边框
    ui->tableWidget->setShowGrid(false); //设置不显示格子线
    ui->tableWidget->verticalHeader()->setVisible(false); //设置垂直头不可见
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);  //设置选择行为时每次选择一行
    ui->tableWidget->setSelectionMode ( QAbstractItemView::SingleSelection); //设置选择模式，选择单行
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers); //设置不可编辑
    ui->tableWidget->setStyleSheet("selection-background-color:lightblue;"); //设置选中背景色
    ui->tableWidget->horizontalHeader()->setStyleSheet("QHeaderView::section{background:skyblue;}"); //设置表头背景色/*

    ui->starttime->setDateTime(QDateTime::currentDateTime());
    ui->endtime->setDateTime(QDateTime::currentDateTime().addSecs(3600));
    //connect(ui->tableWidget,SIGNAL(itemDoubleClicked(QTableWidgetItem*)),this,SLOT(click_to_dispaly(QTableWidgetItem*)));

}

checkout::~checkout()
{
    delete ui;
}

void checkout::load_answer(QVector<dbrow> showoff)
{
    QStringList header;
    header<<tr("房间号")<<tr("开始时间")<<tr("结束时间")<<tr("事件")<<tr("当前温度")<<("目标温度")<<("风速")<<("耗电量")<<("费用");
    ui->tableWidget->setHorizontalHeaderLabels(header);

    int row=0;
    for(int i=0;i<showoff.length();i++){
        ui->tableWidget->insertRow(row);
        ui->tableWidget->setItem(row,0,new QTableWidgetItem(showoff[i].room_id));
        ui->tableWidget->setItem(row,1,new QTableWidgetItem(showoff[i].req_time.toString()));
        ui->tableWidget->setItem(row,2,new QTableWidgetItem(showoff[i].cur_time.toString()));
        ui->tableWidget->setItem(row,3,new QTableWidgetItem(showoff[i].event));
        ui->tableWidget->setItem(row,4,new QTableWidgetItem(QString::number(showoff[i].cur_tp)));
        ui->tableWidget->setItem(row,5,new QTableWidgetItem(QString::number(showoff[i].target_tp)));
        ui->tableWidget->setItem(row,6,new QTableWidgetItem(QString::number(showoff[i].wind_speed)));
        ui->tableWidget->setItem(row,7,new QTableWidgetItem(QString::number(showoff[i].elec)));
        ui->tableWidget->setItem(row,8,new QTableWidgetItem(QString::number(showoff[i].charge)));
        row++;
    }
}

void checkout::clear()
{
    this->ui->tableWidget->clear();
    QStringList header;
    header<<tr("房间号")<<tr("开始时间")<<tr("结束时间")<<tr("事件")<<tr("当前温度")<<("目标温度")<<("风速")<<("耗电量")<<("费用");
    ui->tableWidget->setHorizontalHeaderLabels(header);
}

void checkout::on_search_clicked()
{
    int     protype;
    protype=ui->comboBox->currentIndex();
    int i;
    QVector<dbrow> showoff;
    QString ID;
    QDateTime start_T;
    QDateTime end_T;

    switch (protype) {
        case 0:
            QMessageBox::about(this,"提示","请选择房间号");
            break;
        case 1:
            ID="306C";
            break;
        case 2:
            ID="306D";
            break;
        case 3:
            ID="307C";
            break;
        case 4:
            ID="307D";
            break;
        case 5:
            ID="308C";
            break;
        case 6:
            ID="308D";
            break;
        case 7:
            ID="309C";
            break;
        case 8:
            ID="309D";
            break;
        case 9:
            ID="310C";
            break;
        case 10:
            ID="310D";
            break;
        default:
            break;
    }

    start_T = ui->starttime->dateTime();
    end_T = ui->endtime->dateTime();

    emit query(ID , start_T , end_T);
}


