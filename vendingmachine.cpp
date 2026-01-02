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

VendingMachine::VendingMachine(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::VendingMachine)
    , m_Products{}
{
    ui->setupUi(this);

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
            auto tables = db.tables();
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
                    const QString name = query.value(1).toString();
                    const int price = query.value(2).toInt();
                    const int stock = query.value(3).toInt();

                    m_Products.push_back({ id,name,price,stock });
                }
            }

            QStringList categories = {"음료", "커피", "주스", "기타"};

            for (const auto& categoryName : categories) {
                auto menuLabel = new QLabel(categoryName);
                auto menu = new MenuWidget(categoryName);

                menuLabel->setIndent(10);

                ui->scrollAreaWidgetContents->layout()->addWidget(menuLabel);
                ui->scrollAreaWidgetContents->layout()->addWidget(menu);
            }
        }
    }

    // load price db
    // update price info to product buttons

    // connect button clicked signals to Calculating slots


}

VendingMachine::~VendingMachine()
{
    delete ui;
}
