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
        Change = 3,

        InvalidBalance = 100,
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
    void cashFlow(int amount);

    void logLastTransaction();
    void logTransaction(ErrorCode e, int id);

public slots:
    void OnMenuClicked(MenuItem* btn);

signals:
    // FSM I/O Events
    void sigSelectionProduct(int id);
    void sigCashFlow();

    void sigDone();
    void sigError();

private slots:
    // FSM State Configuration
    void setupStateMachine();

    void enterBooting();
    void enterValidating();
    void enterDebiting();
	void enterCashFlow();
    void enterDispensing();
    void enterSuccess();
    void enterError();

private:
    void showMessageBox(const QString& title, const QString& text);
    void setStatus(const QString& text);

    int getBalance();

private:
    Ui::VendingMachine *ui;

    ProductModel m_ProductModel;
    int m_Balance = 0;
	int m_CashFlowAmount = 0;

	int m_SelectedProductId = -1;
	ErrorCode m_LastError = ErrorCode::Ok;

    // FSM
    QStateMachine m_StateMachine;

    QState* m_Booting = nullptr;
    QState* m_OutofService = nullptr;
    QState* m_Idle = nullptr;
    QState* m_Validating = nullptr;
    QState* m_Debiting = nullptr;
	QState* m_CashFlow = nullptr;
    QState* m_Dispensing = nullptr;
    QState* m_Success = nullptr;
    QState* m_Error = nullptr;

    QTimer m_Timer;
};

#endif // VENDINGMACHINE_H
