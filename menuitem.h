#ifndef MENUITEM_H
#define MENUITEM_H

#include <QToolButton>

#include "productmodel.h"

class MenuItem : public QToolButton
{
    Q_OBJECT

public:
    explicit MenuItem(int id, QWidget *parent = nullptr);
    ~MenuItem() = default;

    int GetId() const {return m_MenuId;}
    void refreshItem(const ProductModel::Product& product);

private:
    int m_MenuId;
};

#endif // MENUITEM_H
