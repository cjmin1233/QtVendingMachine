#include "menuwidget.h"
#include "ui_menuwidget.h"
#include "menuitem.h"

#include <QGridLayout>
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

	// Set up grid layout
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
			// select items by category
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

					// Create a tool button for each menu item
                    // auto btn = new QToolButton(this);
                    // btn->setIcon(QIcon(QString(":/images/%1.jpg").arg(name)));
                    // btn->setIconSize(QSize(120,120));
                    // btn->setText(QString("%1 (%2)").arg(name).arg(price));
                    // btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

                    auto btn = new MenuItem(id, this);
                    btn->setIcon(QIcon(QString(":/images/%1.jpg").arg(name)));
                    btn->setIconSize(QSize(120,120));
                    btn->setText(QString("%1 (%2)").arg(name).arg(price));
                    btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

					// Connect button click signal to slot
                    connect(btn, SIGNAL(clicked()), this, SLOT(on_menuItem_clicked()));

					// Add button to grid layout
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

void MenuWidget::on_menuItem_clicked()
{
    auto btn = qobject_cast<MenuItem*>(sender());
    // qDebug() << btn->text() << " clicked";

    if(btn == nullptr)
    {
        return;
    }

    emit menuItemClicked(btn);
}
