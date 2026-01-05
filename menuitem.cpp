#include "menuitem.h"

MenuItem::MenuItem(int id, QWidget *parent)
    : m_MenuId(id)
    , QToolButton(parent)
{

}

void MenuItem::refreshItem(const ProductModel::Product& product)
{

}
