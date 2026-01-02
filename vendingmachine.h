#ifndef VENDINGMACHINE_H
#define VENDINGMACHINE_H

#include <QWidget>

namespace Ui {
class VendingMachine;
}

class VendingMachine : public QWidget
{
    Q_OBJECT

public:
    explicit VendingMachine(QWidget *parent = nullptr);
    ~VendingMachine();

public slots:


private:
    Ui::VendingMachine *ui;

    int m_balance = 3000;
    // log db, price db
};

#endif // VENDINGMACHINE_H
