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

signals:
    // FSM I/O Events
    void sigSelectionProduct(int id);
    void sigValidationDone();
    void sigValidationFailed(ErrorCode e);
    void sigDebitingDone();
    void sigDispenseDone();
    void sigDispenseFailed(ErrorCode e);

    void sigShowMessage();
    void sigClearMessage();

private:
    // FSM State Configuration
    void setupStateMachine();

    void enterBooting();
    void enterValidating();
    void enterDebiting();
    void enterDispensing();
    void enterSuccess();
    void enterError(ErrorCode e);    

private:
    Ui::VendingMachine *ui;

    ProductModel m_ProductModel;
    int m_balance = 3000;

	int m_SelectedProductId = -1;
	ErrorCode m_LastError = ErrorCode::Ok;

    // FSM
    QStateMachine m_StateMachine;
    QState* m_Booting = nullptr;
    QState* m_OutofService = nullptr;
    QState* m_Idle = nullptr;
    QState* m_Validating = nullptr;
    QState* m_Debiting = nullptr;
    QState* m_Dispensing = nullptr;
    QState* m_Success = nullptr;
    QState* m_Error = nullptr;

    QTimer m_FeedbackTimer;
};

#endif // VENDINGMACHINE_H
