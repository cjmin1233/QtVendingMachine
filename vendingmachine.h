#ifndef VENDINGMACHINE_H
#define VENDINGMACHINE_H

#include <QWidget>
#include <QString>

#include "productmodel.h"

namespace Ui {
class VendingMachine;
}

class MenuItem;

class VendingMachine : public QWidget
{
    Q_OBJECT

public:
    enum class ErrorCode
    {
        Ok = 0,

        InsufficientBalance = 100,
        OutOfStock = 101,
        InvalidProduct = 102,

        DatabaseError = 400,
    };

public:
    explicit VendingMachine(QWidget *parent = nullptr);
    ~VendingMachine();

    ErrorCode canSell(int id) const;
    void dispense(int id);
    void logTransaction(ErrorCode e, int id);

public slots:
    void OnMenuClicked(MenuItem* btn);

private:
    Ui::VendingMachine *ui;

    ProductModel m_ProductModel;
    int m_balance = 1500;
};

#endif // VENDINGMACHINE_H
