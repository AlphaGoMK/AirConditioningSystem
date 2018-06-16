#ifndef CHECKOUT_H
#define CHECKOUT_H
#include <QMessageBox>
#include <QDialog>
#include "server.h"

namespace Ui {
class checkout;
}

struct dbrow;

class checkout : public QDialog
{
    Q_OBJECT

public:
    explicit checkout(QWidget *parent = 0);
    ~checkout();
    void load_answer(QVector<dbrow>);
    void clear();
private slots:
    void on_search_clicked();

signals:
    void query(QString, QDateTime, QDateTime);

private:
    Ui::checkout *ui;
};

#endif // CHECKOUT_H
