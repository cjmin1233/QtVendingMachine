#ifndef MENUWIDGET_H
#define MENUWIDGET_H

#include <QWidget>
#include <QString>

#include "productmodel.h"

class MenuItem;

namespace Ui {
class MenuWidget;
}

class MenuWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MenuWidget(const QString& categoryName, const ProductModel& model, QWidget *parent = nullptr);
    ~MenuWidget();

    void clearLayout();
    void refresh();
    void refreshItem(int id);

signals:
    void menuItemClicked(MenuItem* btn);

private slots:
    void OnMenuClicked();
    //void OnProductsChanged(const QString& category){if(category==m_CategoryName)refresh();}

private:
    Ui::MenuWidget *ui;

    QString m_CategoryName;
    const ProductModel& m_RefModel;
};

#endif // MENUWIDGET_H
