#include "vendingmachine.h"
#include "ui_vendingmachine.h"

#include <QGridLayout>
#include <QLabel>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QDateTime>

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
    default:
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

    {
        // load products from database
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", QString("VendingMachine"));
        db.setDatabaseName("maindb.db");

        if (!db.open())
        {
            qWarning() << "DB open failed: " << db.lastError().text();
        }
        else
        {
            QSqlQuery query(db);
            if (!query.exec("select * from priceTable order by id asc"))
            {
                qWarning() << "query failed: " << query.lastError().text();
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

                    ProductModel::Product p={id,name,price,stock,category};
                    m_ProductModel.insert(p);
                }
            }

			// define category order
            QStringList categoryOrder = {"음료", "커피", "주스", "기타"};

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
        }
    }
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

    // state inspect
    {

    }

    return ErrorCode::Ok;
}

void VendingMachine::dispense(int id)
{
    auto& product = m_ProductModel.products()[id];

    // dispense...
    QSqlDatabase db = QSqlDatabase::database("VendingMachine");
    if(!db.open())
    {
        qWarning() << "DB not opened";
        qDebug() << (int)ErrorCode::DatabaseError;
        logTransaction(ErrorCode::DatabaseError, id);
        return;
    }

    QSqlQuery query(db);
    query.prepare(
        "UPDATE priceTable "
        "SET stock = stock - 1 "
        "WHERE id = :id AND stock > 0"
        );
    query.bindValue(":id",id);

    if(!query.exec())
    {
        qWarning() << "DB update failed: "<<query.lastError().text();
        qDebug() << (int)ErrorCode::DatabaseError;
        logTransaction(ErrorCode::DatabaseError, id);
        return;
    }

    if(query.numRowsAffected() == 0)
    {
        qWarning() << "Dispense failed: no stock or invalid id";
        qDebug() << (int)ErrorCode::DatabaseError;
        logTransaction(ErrorCode::DatabaseError, id);
        return;
    }


    // after dispense
    --product.stock;
    emit m_ProductModel.productsChanged(product);
    qDebug()<<(int)ErrorCode::Ok;
    logTransaction(ErrorCode::Ok, id);
}

void VendingMachine::OnMenuClicked(MenuItem* btn)
{
    int itemID = btn->GetId();

    ErrorCode errCode = canSell(itemID);
    if(errCode == ErrorCode::Ok)
    {
        dispense(itemID);
    }
    else
    {
        qDebug() << (int)errCode;
        logTransaction(errCode, itemID);
    }
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
        qDebug() << (int)ErrorCode::DatabaseError;
        return;
    }
}
