#ifndef VENDINGMACHINE_H
#define VENDINGMACHINE_H

#include <QWidget>
#include <QString>
#include <QStateMachine>
#include <QState>
#include <QFinalState>
#include <QTimer>

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
        Debit = 1,
        Deposit = 2,

        InsufficientBalance = 100,
        OutOfStock = 101,
        InvalidProduct = 102,

        DatabaseError = 400,
    };

public:
    explicit VendingMachine(QWidget *parent = nullptr);
    ~VendingMachine();

    ErrorCode canSell(int id) const;
    void dispense();
    void debit();
    void deposit(int amount);

    void logLastTransaction();
    void logTransaction(ErrorCode e, int id);

public slots:
    void OnMenuClicked(MenuItem* btn);

signals:
    // FSM I/O Events
    void sigSelectionProduct(int id);
    void sigDeposit();

    void sigDone();
    void sigError();

private:
    // FSM State Configuration
    void setupStateMachine();

    void enterBooting();
    void enterValidating();
    void enterDebiting();
	void enterDepositing();
    void enterDispensing();
    void enterSuccess();
    void enterError();

    void showMessageBox(const QString& title, const QString& text);
    void setStatus(const QString& text);

    int getBalance();

private:
    Ui::VendingMachine *ui;

    ProductModel m_ProductModel;
    int m_Balance = 0;

	int m_SelectedProductId = -1;
	ErrorCode m_LastError = ErrorCode::Ok;

    // FSM
    QStateMachine m_StateMachine;
    QState* m_Booting = nullptr;
    QState* m_OutofService = nullptr;
    QState* m_Idle = nullptr;
    QState* m_Validating = nullptr;
    QState* m_Debiting = nullptr;
	QState* m_Depositing = nullptr;
    QState* m_Dispensing = nullptr;
    QState* m_Success = nullptr;
    QState* m_Error = nullptr;

    QTimer m_Timer;
};

#endif // VENDINGMACHINE_H
