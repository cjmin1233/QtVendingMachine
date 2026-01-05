#include "menuwidget.h"
#include "ui_menuwidget.h"
#include "menuitem.h"

#include <QGridLayout>
#include <QToolButton>

#include <QDebug>

MenuWidget::MenuWidget(const QString& categoryName, const ProductModel& model, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MenuWidget)
    , m_CategoryName(categoryName)
    , m_RefModel(model)
{
    ui->setupUi(this);

	// Set up grid layout
    auto gridLayout = new QGridLayout(this);
    gridLayout->setAlignment(Qt::AlignLeft);
    setLayout(gridLayout);

    connect(&m_RefModel, &ProductModel::productsChanged,
            this, [this](const ProductModel::Product& product)
            {
                if(product.category == m_CategoryName)
                {
                    refreshItem(product);
                }
            });

    refresh();
}

MenuWidget::~MenuWidget()
{
    delete ui;
}

void MenuWidget::clearLayout()
{
    auto curLayout = this->layout();
    while(auto item = curLayout->takeAt(0))
    {
        if(QWidget* w = item->widget())
        {
            w->deleteLater();
        }
        delete item;
    }
}

// Refresh the menu items based on the current category
void MenuWidget::refresh()
{
	// Clear existing items in the layout
	auto gridLayout = qobject_cast<QGridLayout*>(layout());
    if(gridLayout == nullptr)
    {
        return;
	}

    clearLayout();

    int index=0;
    const int columns=4;

    const auto& products = m_RefModel.products();
    for(const auto& product : products)
    {
        if(product.category != m_CategoryName)
        {
            continue;
        }

        const int id = product.id;
        const QString& name = product.name;
        const int price = product.price;
        const int stock = product.stock;

        auto menuItem = new MenuItem(id, this);
        menuItem->setIcon(QIcon(QString(":/images/%1.jpg").arg(name)));
        menuItem->setIconSize(QSize(120,120));
        menuItem->setText(QString("%1 (%2)\n재고: %3").arg(name).arg(price).arg(stock));
        menuItem->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

        // Connect button click signal to slot
        connect(menuItem, SIGNAL(clicked()), this, SLOT(OnMenuClicked()));

        // Add button to grid layout
        int row = index / columns;
        int col = index % columns;
        gridLayout->addWidget(menuItem, row, col);

        ++index;
    }
}

// Refresh a specific menu item by its ID
void MenuWidget::refreshItem(const ProductModel::Product& product)
{
    auto gridLayout = qobject_cast<QGridLayout*>(layout());
    if(gridLayout == nullptr)
    {
        return;
    }

    for(int i=0;i<gridLayout->count();++i)
    {
        QLayoutItem* item = gridLayout->itemAt(i);
        if(item == nullptr)
        {
            continue;
        }

        auto menuItem = qobject_cast<MenuItem*>(item->widget());
        if(menuItem == nullptr)
        {
            continue;
        }

        if(menuItem->GetId() == product.id)
        {
            menuItem->setText(QString("%1 (%2)\n재고: %3").arg(product.name).arg(product.price).arg(product.stock));
            menuItem->setEnabled(product.stock > 0);
            return;
        }
    }
}

void MenuWidget::OnMenuClicked()
{
    auto btn = qobject_cast<MenuItem*>(sender());

    if(btn == nullptr)
    {
        return;
    }

    emit menuItemClicked(btn);
}
