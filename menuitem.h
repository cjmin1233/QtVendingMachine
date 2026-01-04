#ifndef MENUITEM_H
#define MENUITEM_H

#include <QToolButton>

class MenuItem : public QToolButton
{
    Q_OBJECT

public:
    explicit MenuItem(int id, QWidget *parent = nullptr);
    ~MenuItem() = default;

    int GetId() const {return m_MenuId;}

private:
    int m_MenuId;
};

#endif // MENUITEM_H
