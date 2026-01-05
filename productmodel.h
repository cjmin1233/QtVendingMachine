#ifndef PRODUCTMODEL_H
#define PRODUCTMODEL_H

#include <QObject>
#include <QMap>

class ProductModel : public QObject
{
public:
    struct Product
    {
        int id;
        QString name;
        int price;
        int stock;
        QString category;
    };

    Q_OBJECT
public:
    explicit ProductModel(QObject *parent = nullptr);

    void insert(const Product& product) {m_Products[product.id] = product; emit productsChanged(product.category, product.id);}

    QMap<int, Product>& products() {return m_Products;}
    const QMap<int, Product>& products() const {return m_Products;}

signals:
    // value changed
    void productsChanged(const QString& category, int id);

private:
    QMap<int, Product> m_Products;
};

#endif // PRODUCTMODEL_H
