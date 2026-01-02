#ifndef VENDINGMACHINE_H
#define VENDINGMACHINE_H

#include <QWidget>
#include <QString>

namespace Ui {
class VendingMachine;
}


class VendingMachine : public QWidget
{
    struct Product
    {
        int id;
        QString name;
        int price;
        int stock;
    };

    Q_OBJECT

public:
    explicit VendingMachine(QWidget *parent = nullptr);
    ~VendingMachine();

public slots:


private:
    Ui::VendingMachine *ui;

    int m_balance = 3000;

    QVector<Product> m_Products;
};

#endif // VENDINGMACHINE_H
