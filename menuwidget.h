#ifndef MENUWIDGET_H
#define MENUWIDGET_H

#include <QWidget>
#include <QString>

class QToolButton;
class QImage;
class MenuItem;

namespace Ui {
class MenuWidget;
}

class MenuWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MenuWidget(const QString& categoryName, QWidget *parent = nullptr);
    ~MenuWidget();

signals:
    void menuItemClicked(MenuItem* btn);

private slots:
    void on_menuItem_clicked();

private:
    Ui::MenuWidget *ui;

    QString m_CategoryName;
};

#endif // MENUWIDGET_H
