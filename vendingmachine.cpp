#include "vendingmachine.h"
#include "ui_vendingmachine.h"

#include <QGridLayout>
#include <QPushButton>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

VendingMachine::VendingMachine(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::VendingMachine)
    , m_Products{}
{
    ui->setupUi(this);

    // setup grid layout
    const int columns = 4;
    auto* gridLayout = new QGridLayout(this);
    gridLayout->setContentsMargins(8, 8, 8, 8);
    gridLayout->setSpacing(8);
    setLayout(gridLayout);

    {
        // load products from database
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
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
        }
    }

    // create product buttons
    for (qsizetype i = 0; i < m_Products.size(); ++i)
    {
        const int row = i / columns;
        const int col = i % columns;

        const QString& name = m_Products[i].name;
        auto* btn = new QPushButton(name.isEmpty() ? QString("product %1").arg(i + 1) : name, this);

        QString safeName = name;
        safeName.replace(' ', '_');
        btn->setObjectName(QString("button_%1_%2").arg(i + 1).arg(safeName));

        gridLayout->addWidget(btn, row, col);
    }


    // load price db
    // update price info to product buttons

    // connect button clicked signals to Calculating slots


}

VendingMachine::~VendingMachine()
{
    delete ui;
}
