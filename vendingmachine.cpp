#include "vendingmachine.h"
#include "ui_vendingmachine.h"

#include <QGridLayout>
#include <QPushButton>
#include <QLabel>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

#include "menuwidget.h"
#include "menuitem.h"

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

void VendingMachine::OnMenuClicked(MenuItem* btn)
{
    int itemID = btn->GetId();
    auto& product = m_ProductModel.products()[itemID];

    // inspect balance, stock
    if(product.stock>0 && m_balance >= product.price)
    {
        // if OK, dispense
        --product.stock;
        qDebug()<<"balance: "<<m_balance;

        // after dispense
        emit m_ProductModel.productsChanged(product.category, product.id);
    }
    // else, return error
    else
    {

    }
}
