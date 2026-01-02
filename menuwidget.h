#ifndef MENUWIDGET_H
#define MENUWIDGET_H

#include <QWidget>
#include <QString>

class QPushButton;
class QImage;

namespace Ui {
class MenuWidget;
}

class MenuWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MenuWidget(const QString& categoryName, QWidget *parent = nullptr);
    ~MenuWidget();

private:
    Ui::MenuWidget *ui;

    QString m_CategoryName;
};

#endif // MENUWIDGET_H
