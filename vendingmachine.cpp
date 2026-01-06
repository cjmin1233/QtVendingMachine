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

static QString ErrorCode2String(VendingMachine::ErrorCode e)
{
    QString description;
    switch (e)
    {
    case VendingMachine::ErrorCode::Ok:
        description = "정상 판매.";
        break;
    case VendingMachine::ErrorCode::InsufficientBalance:
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
{
    ui->setupUi(this);

	// Set up scroll area layout
    auto layout = new QVBoxLayout(ui->scrollAreaWidgetContents);
    layout->setSpacing(10);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignLeft);

    ui->scrollAreaWidgetContents->setLayout(layout);

    setupStateMachine();
    m_StateMachine.start();
}

VendingMachine::~VendingMachine()
{
    delete ui;
}

VendingMachine::ErrorCode VendingMachine::canSell(int id) const
{
    if(!m_ProductModel.products().contains(id))
    {
        return ErrorCode::InvalidProduct;
    }

    const auto& product = m_ProductModel.products()[id];
    if(product.stock<=0)
    {
        return ErrorCode::OutOfStock;
    }

    if(m_balance<product.price)
    {
        return ErrorCode::InsufficientBalance;
    }

    return ErrorCode::Ok;
}

void VendingMachine::dispense()
{
    QSqlDatabase db = QSqlDatabase::database("VendingMachine");
    if (!db.open())
    {
        m_LastError = ErrorCode::DatabaseError;

        emit sigError();
        return;
    }

    QSqlQuery query(db);
    query.prepare(
        "UPDATE priceTable "
        "SET stock = stock - 1 "
        "WHERE id = :id AND stock > 0"
        );
    query.bindValue(":id", m_SelectedProductId);

    if (!query.exec() || query.numRowsAffected() == 0)
    {
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
void VendingMachine::logTransaction(ErrorCode e, int id)
{
    QSqlDatabase db = QSqlDatabase::database("VendingMachine");
    if(!db.open())
    {
        qDebug() << "Log db open failed";
        return;
    }

    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO logTable "
        "(log_time, product_id, error_code, description) "
        "VALUES (:log_time, :product_id, :error_code, :description)"
        );

    query.bindValue(":log_time", QDateTime::currentDateTime());
    query.bindValue(":product_id", id);
    query.bindValue(":error_code", static_cast<int>(e));
    query.bindValue(":description", ErrorCode2String(e));

    if(!query.exec())
    {
        qWarning() << "DB update failed: "<<query.lastError().text();
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
    m_Dispensing = new QState();
    m_Success = new QState();
    m_Error = new QState();

	// Add states to the state machine
	m_StateMachine.addState(m_Booting);
	m_StateMachine.addState(m_OutofService);
	m_StateMachine.addState(m_Idle);
	m_StateMachine.addState(m_Validating);
	m_StateMachine.addState(m_Debiting);
	m_StateMachine.addState(m_Dispensing);
	m_StateMachine.addState(m_Success);
	m_StateMachine.addState(m_Error);

	// Set the initial state
	m_StateMachine.setInitialState(m_Booting);

	// Connect state entered signals to corresponding slots or lambdas
    connect(m_Booting, &QState::entered,
        this, &VendingMachine::enterBooting);
    connect(m_Idle, &QState::entered,
            [this](){ui->scrollArea->setEnabled(true);});
    connect(m_Idle, &QState::exited,
            [this](){ui->scrollArea->setEnabled(false);});

    connect(m_Validating, &QState::entered,
            this, &VendingMachine::enterValidating);
    connect(m_Debiting, &QState::entered,
            this, &VendingMachine::enterDebiting);
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

    // Validating state transitions
    m_Validating->addTransition(this, &VendingMachine::sigDone,
        m_Debiting);
    m_Validating->addTransition(this, &VendingMachine::sigError,
        m_Error);

    // Debiting state transitions
    m_Debiting->addTransition(this, &VendingMachine::sigDone,
        m_Dispensing);

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
    // load products from database
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", QString("VendingMachine"));
    db.setDatabaseName("maindb.db");

    if (!db.open())
    {
        qWarning() << "DB open failed: " << db.lastError().text();
        m_LastError = ErrorCode::DatabaseError;

        emit sigError();
        return;
    }
    else
    {
        QSqlQuery query(db);
        if (!query.exec("select * from priceTable order by id asc"))
        {
            qWarning() << "query failed: " << query.lastError().text();
            m_LastError = ErrorCode::DatabaseError;

            emit sigError();
            return;
        }
        else
        {
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

        ui->labelBalance->setText(QString("잔액: %1 원").arg(m_balance));

        // Transition to Idle state
        emit sigDone();
    }
}

void VendingMachine::enterValidating()
{
	const ErrorCode e = canSell(m_SelectedProductId);

    if(e == ErrorCode::Ok)
    {
        emit sigDone();
    }
    else
    {
        m_LastError = e;

        emit sigError();
	}
}

void VendingMachine::enterDebiting()
{
	const auto& product = m_ProductModel.products()[m_SelectedProductId];
    m_balance -= product.price;
    ui->labelBalance->setText(QString("잔액: %1 원").arg(m_balance));

    emit sigDone();
}

void VendingMachine::enterDispensing()
{
    dispense();
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

    showMessageBox("판매 실패", ErrorCode2String(m_LastError));
    emit sigDone();
}

void VendingMachine::showMessageBox(const QString& title, const QString& text)
{
    QMessageBox::information(this, title, text);
}
