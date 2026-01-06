#include "vendingmachine.h"
#include "ui_vendingmachine.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QDateTime>
#include <QLabel>
#include <QMessageBox>

#include "menuwidget.h"
#include "menuitem.h"
#include "DatabaseUtils.h"

// Convert ErrorCode to description string
static QString ErrorCode2String(VendingMachine::ErrorCode e)
{
    QString description;
    switch (e)
    {
    case VendingMachine::ErrorCode::Ok:
        description = "정상 판매.";
        break;
    case VendingMachine::ErrorCode::Debit:
        description = "잔액 차감.";
        break;
    case VendingMachine::ErrorCode::Deposit:
        description = "입금.";
        break;
    case VendingMachine::ErrorCode::Change:
        description = "잔돈 반환.";
        break;
    case VendingMachine::ErrorCode::InvalidBalance:
        description = "잔액 부족.";
        break;
    case VendingMachine::ErrorCode::OutOfStock:
        description = "재고 부족.";
        break;
    case VendingMachine::ErrorCode::InvalidProduct:
        description = "유효하지 않은 물품.";
        break;
    case VendingMachine::ErrorCode::DatabaseError:
        description = "데이터베이스 에러.";
        break;
    }

    return description;
}

VendingMachine::VendingMachine(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::VendingMachine)
    , m_ProductModel(this)
    , m_StateMachine(this)
    , m_Timer(this)
{
    ui->setupUi(this);

    // Set up scroll area layout
    auto layout = new QVBoxLayout(ui->scrollAreaWidgetContents);
    layout->setSpacing(10);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignLeft);

    ui->scrollAreaWidgetContents->setLayout(layout);

	// Connect deposit and change buttons
    connect(ui->btnDeposit, &QPushButton::clicked,
        [this]() {m_CashFlowAmount = 1000; emit sigCashFlow(); });
    connect(ui->btnChange, &QPushButton::clicked,
        [this]()
        {
            m_CashFlowAmount = -m_Balance;
            if (m_CashFlowAmount != 0)
            {
                emit sigCashFlow();
            }
        });

    setupStateMachine();
    m_StateMachine.start();

    m_Timer.setSingleShot(true);
}

VendingMachine::~VendingMachine()
{
    delete ui;
}

// check if product can be sold
VendingMachine::ErrorCode VendingMachine::canSell(int id) const
{
    // check if product exists
    if (!m_ProductModel.products().contains(id))
    {
        return ErrorCode::InvalidProduct;
    }

    // check stock and balance
    const auto& product = m_ProductModel.products()[id];
    if (product.stock <= 0)
    {
        return ErrorCode::OutOfStock;
    }

    if (m_Balance < product.price)
    {
        return ErrorCode::InvalidBalance;
    }

    return ErrorCode::Ok;
}

// dispense product (deduct stock by 1)
void VendingMachine::dispense()
{
    QSqlDatabase db = Db::database();
    QSqlQuery query(db);

    query.prepare(
        "UPDATE priceTable "
        "SET stock = stock - 1 "
        "WHERE id = :id AND stock > 0"
    );

    query.bindValue(":id", m_SelectedProductId);

    if (!Db::begin() || !query.exec() || !Db::commit())
    {
        Db::rollback();

        m_LastError = ErrorCode::DatabaseError;
        emit sigError();
        return;
    }

    // after dispense
    auto& product = m_ProductModel.products()[m_SelectedProductId];
    --product.stock;
    m_LastError = ErrorCode::Ok;

    emit m_ProductModel.productsChanged(product);
    emit sigDone();
}

// deduct product price from balance
void VendingMachine::debit()
{
    QSqlDatabase db = Db::database();
    QSqlQuery query(db);

    query.prepare(
        "INSERT INTO logTable "
        "(log_time, product_id, balance, error_code, description) "
        "VALUES (:log_time, :product_id, :balance, :error_code, :description)"
    );

    const auto& product = m_ProductModel.products()[m_SelectedProductId];
    m_LastError = ErrorCode::Debit;

    query.bindValue(":log_time", QDateTime::currentDateTime());
    query.bindValue(":product_id", m_SelectedProductId);
    query.bindValue(":balance", m_Balance - product.price);
    query.bindValue(":error_code", static_cast<int>(m_LastError));
    query.bindValue(":description", ErrorCode2String(m_LastError));

    if (!Db::begin() || !query.exec() || !Db::commit())
    {
        Db::rollback();

        m_LastError = ErrorCode::DatabaseError;
        emit sigError();
        return;
    }

	// update balance
    m_Balance = getBalance();
    ui->labelBalance->setText(QString("잔액: %1 원").arg(m_Balance));

    emit sigDone();
}

// change balance by amount (can be negative)
void VendingMachine::cashFlow(int amount)
{
    // check for insufficient balance
    int newBalance = m_Balance + amount;
    if (newBalance < 0)
    {
        m_LastError = ErrorCode::InvalidBalance;
        emit sigError();
        return;
    }

    QSqlDatabase db = Db::database();
    QSqlQuery query(db);

    query.prepare("INSERT INTO logTable "
        "(log_time, product_id, balance, error_code, description) "
        "VALUES (:log_time, :product_id, :balance, :error_code, :description)");

    query.bindValue(":log_time", QDateTime::currentDateTime());
    query.bindValue(":product_id", 0);
    query.bindValue(":balance", newBalance);
    query.bindValue(":error_code", static_cast<int>(m_LastError));
    query.bindValue(":description", ErrorCode2String(m_LastError));

    if (!Db::begin() || !query.exec() || !Db::commit())
    {
        Db::rollback();

        m_LastError = ErrorCode::DatabaseError;
        emit sigError();
        return;
    }

	// update balance
    m_Balance = getBalance();
    ui->labelBalance->setText(QString("잔액: %1 원").arg(m_Balance));

    emit sigDone();
}

void VendingMachine::OnMenuClicked(MenuItem* btn)
{
    int itemID = btn->GetId();
    m_SelectedProductId = itemID;

    emit sigSelectionProduct(itemID);
}

void VendingMachine::logLastTransaction()
{
    logTransaction(m_LastError, m_SelectedProductId);
}
// log transaction with given error code and product id
void VendingMachine::logTransaction(ErrorCode e, int id)
{
    QSqlDatabase db = Db::database();
    if (!db.isOpen())
    {
        qDebug() << "Log db open failed";
        return;
    }

    // Begin database transaction
    if (!Db::begin())
    {
        qDebug() << "Log db begin transaction failed";

        return;
    }

    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO logTable "
        "(log_time, product_id, balance, error_code, description) "
        "VALUES (:log_time, :product_id, :balance, :error_code, :description)"
    );

    query.bindValue(":log_time", QDateTime::currentDateTime());
    query.bindValue(":product_id", id);
    query.bindValue(":balance", m_Balance);
    query.bindValue(":error_code", static_cast<int>(e));
    query.bindValue(":description", ErrorCode2String(e));

    // Execute the query, check for success
    if (!query.exec() || query.numRowsAffected() == 0)
    {
        Db::rollback();
        qWarning() << "DB update failed: " << query.lastError().text();

        return;
    }

    // Commit transaction
    if (!Db::commit())
    {
        Db::rollback();
        qWarning() << "DB update failed: " << query.lastError().text();

        return;
    }
}

void VendingMachine::setupStateMachine()
{
    // Initialize states
    m_Booting = new QState();
    m_OutofService = new QState();
    m_Idle = new QState();

    m_Validating = new QState();
    m_Debiting = new QState();
    m_CashFlow = new QState();
    m_Dispensing = new QState();

    m_Success = new QState();
    m_Error = new QState();

    // Add states to the state machine
    m_StateMachine.addState(m_Booting);
    m_StateMachine.addState(m_OutofService);
    m_StateMachine.addState(m_Idle);

    m_StateMachine.addState(m_Validating);
    m_StateMachine.addState(m_Debiting);
    m_StateMachine.addState(m_CashFlow);
    m_StateMachine.addState(m_Dispensing);

    m_StateMachine.addState(m_Success);
    m_StateMachine.addState(m_Error);

    // Set the initial state
    m_StateMachine.setInitialState(m_Booting);

    // Connect state signals to corresponding slots or lambdas
    connect(m_Booting, &QState::entered,
        this, &VendingMachine::enterBooting);
    connect(m_Idle, &QState::entered,
        [this]()
        {
            ui->scrollArea->setEnabled(true);
            setStatus(QStringLiteral("대기 중 - 상품을 선택하세요."));
        });
    connect(m_Idle, &QState::exited,
        [this]()
        {
            ui->scrollArea->setEnabled(false);
        });
    connect(m_OutofService, &QState::entered,
        [this]() {setStatus(QStringLiteral("점검 중(Out of Service)")); });

    connect(m_Validating, &QState::entered,
        this, &VendingMachine::enterValidating);
    connect(m_Debiting, &QState::entered,
        this, &VendingMachine::enterDebiting);
    connect(m_CashFlow, &QState::entered,
        this, &VendingMachine::enterCashFlow);
    connect(m_Dispensing, &QState::entered,
        this, &VendingMachine::enterDispensing);

    connect(m_Success, &QState::entered,
        this, &VendingMachine::enterSuccess);
    connect(m_Error, &QState::entered,
        this, &VendingMachine::enterError);

    // Booting state transitions
    m_Booting->addTransition(this, &VendingMachine::sigError,
        m_OutofService);
    m_Booting->addTransition(this, &VendingMachine::sigDone,
        m_Idle);

    // Idle state transitions
    m_Idle->addTransition(this, &VendingMachine::sigSelectionProduct,
        m_Validating);
    m_Idle->addTransition(this, &VendingMachine::sigCashFlow,
        m_CashFlow);

    // Validating state transitions
    m_Validating->addTransition(this, &VendingMachine::sigDone,
        m_Debiting);
    m_Validating->addTransition(this, &VendingMachine::sigError,
        m_Error);

    // Debiting state transitions
    m_Debiting->addTransition(this, &VendingMachine::sigDone,
        m_Dispensing);

    // CashFlow state transitions
    m_CashFlow->addTransition(this, &VendingMachine::sigDone,
        m_Idle);
    m_CashFlow->addTransition(this, &VendingMachine::sigError,
        m_Error);

    // Dispensing state transitions
    m_Dispensing->addTransition(this, &VendingMachine::sigDone,
        m_Success);
    m_Dispensing->addTransition(this, &VendingMachine::sigError,
        m_Error);

    // Success and Error state transitions
    m_Success->addTransition(this, &VendingMachine::sigDone,
        m_Idle);
    m_Error->addTransition(this, &VendingMachine::sigDone,
        m_Idle);
}

void VendingMachine::enterBooting()
{
    setStatus(QStringLiteral("부팅 중..."));

    // load products from database
    QSqlDatabase db = Db::database();
    if (!db.isOpen())
    {
        qWarning() << "DB open failed: " << db.lastError().text();
        m_LastError = ErrorCode::DatabaseError;

        emit sigError();
        return;
    }

    QSqlQuery query(db);
    if (!query.exec("select * from priceTable order by id asc"))
    {
        qWarning() << "query failed: " << query.lastError().text();
        m_LastError = ErrorCode::DatabaseError;

        emit sigError();
        return;
    }

    // populate product model
    while (query.next())
    {
        const int id = query.value(0).toInt();
        const QString& name = query.value(1).toString();
        const int price = query.value(2).toInt();
        const int stock = query.value(3).toInt();
        const QString& category = query.value(4).toString();

        ProductModel::Product p = { id,name,price,stock,category };
        m_ProductModel.insert(p);
    }

    // define category order
    QStringList categoryOrder = { "음료", "커피", "주스", "기타" };

    // create menu widgets for each category
    for (const auto& categoryName : categoryOrder)
    {
        auto menuLabel = new QLabel(categoryName);
        auto menu = new MenuWidget(categoryName, m_ProductModel);

        menuLabel->setIndent(10);
        // connect menu item clicked signal to slot
        connect(menu, SIGNAL(menuItemClicked(MenuItem*)), this, SLOT(OnMenuClicked(MenuItem*)));

        ui->scrollAreaWidgetContents->layout()->addWidget(menuLabel);
        ui->scrollAreaWidgetContents->layout()->addWidget(menu);
    }

    // Initialize balance text
    m_Balance = getBalance();
    ui->labelBalance->setText(QString("잔액: %1 원").arg(m_Balance));

    // Transition to Idle state
    emit sigDone();
}

void VendingMachine::enterValidating()
{
    setStatus(QStringLiteral("선택한 상품 확인 중..."));

	// check if product can be sold
    const ErrorCode e = canSell(m_SelectedProductId);

    if (e == ErrorCode::Ok)
    {
        m_Timer.singleShot(400, this, [this]() {emit sigDone(); });
    }
    else
    {
        m_LastError = e;

        m_Timer.singleShot(400, this, [this]() {emit sigError(); });
    }
}

void VendingMachine::enterDebiting()
{
    setStatus(QStringLiteral("결제 처리 중..."));

    m_Timer.singleShot(500, this, [this]() { debit(); });
}

void VendingMachine::enterCashFlow()
{
    setStatus(QStringLiteral("잔액 처리 중..."));

	// set last error code based on cash flow amount
    m_LastError = m_CashFlowAmount > 0 ? ErrorCode::Deposit : ErrorCode::Change;
    m_Timer.singleShot(500, this, [this]() { cashFlow(m_CashFlowAmount); });
}

void VendingMachine::enterDispensing()
{
    setStatus(QStringLiteral("물품 배출중..."));

    m_Timer.singleShot(1200, this, [this]() { dispense(); });
}

void VendingMachine::enterSuccess()
{
    logLastTransaction();

    showMessageBox("판매 완료", "상품이 정상적으로 배출되었습니다.");
    emit sigDone();
}

void VendingMachine::enterError()
{
    logLastTransaction();

    showMessageBox("실패", ErrorCode2String(m_LastError));
    emit sigDone();
}

void VendingMachine::showMessageBox(const QString& title, const QString& text)
{
    QMessageBox::information(this, title, text);
}

void VendingMachine::setStatus(const QString& text)
{
    ui->labelStatus->setText(text);
}

// Get the latest balance from the log table
int VendingMachine::getBalance()
{
    QSqlDatabase db = Db::database();

    if (!db.isOpen())
    {
        return 0;
    }

    QSqlQuery query(db);
    if (!query.exec("SELECT balance FROM logTable ORDER BY log_time DESC LIMIT 1"))
    {
        return 0;
    }

    if (query.next())
    {
        const int latestBalance = query.value(0).toInt();
        return latestBalance;
    }

    return 0;
}
