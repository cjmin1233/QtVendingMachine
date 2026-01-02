#include "menuwidget.h"
#include "ui_menuwidget.h"

#include <QGridLayout>
#include <QPushButton>
#include <QToolButton>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDebug>

MenuWidget::MenuWidget(const QString& categoryName, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MenuWidget)
    , m_CategoryName(categoryName)
{
    ui->setupUi(this);

    auto gridLayout = new QGridLayout(this);
    gridLayout->setAlignment(Qt::AlignLeft);
    setLayout(gridLayout);

    {
        // load products from database
        QSqlDatabase db = QSqlDatabase::database(QString("VendingMachine"));

        if (!db.open())
        {
            // qWarning() << "DB open failed: " << db.lastError().text();
        }
        else
        {
            QSqlQuery query(QString("SELECT * "
                                    "FROM priceTable "
                                    "WHERE category = %1 "
                                    "ORDER BY id ASC ").arg("\'" + m_CategoryName + "\'"), db);
            if (!query.exec())
            {
                // qWarning() << "query failed: " << query.lastError().text();
            }
            else
            {
                int index=0;
                const int columns=4;
                while (query.next())
                {
                    const int id = query.value(0).toInt();
                    const QString name = query.value(1).toString();
                    const int price = query.value(2).toInt();
                    const int stock = query.value(3).toInt();

                    auto btn = new QToolButton(this);
                    btn->setIcon(QIcon(QString(":/images/%1.jpg").arg(name)));
                    btn->setIconSize(QSize(120,120));
                    btn->setText(QString("%1 (%2)").arg(name).arg(price));
                    btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

                    gridLayout->addWidget(btn, index/columns, index%columns);

                    ++index;
                }
            }
        }
    }
}

MenuWidget::~MenuWidget()
{
    delete ui;
}
