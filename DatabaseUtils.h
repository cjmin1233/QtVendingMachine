#ifndef DATABASEUTILS_H
#define DATABASEUTILS_H

#include <QString>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

namespace Db
{
inline const QString connectionName = QStringLiteral("VendingMachine");

inline QSqlDatabase database()
{
    QSqlDatabase db = QSqlDatabase::database(connectionName);

    if(!db.isValid())
    {
        db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        db.setDatabaseName(QStringLiteral("maindb.db"));
    }

    if(!db.isOpen())
    {
        db.open();
    }

    return db;
}

inline bool begin()
{
    QSqlDatabase db = database();
    return db.transaction();
}

inline bool commit()
{
    QSqlDatabase db = database();
    return db.commit();
}

inline void rollback()
{
    QSqlDatabase db = database();
    db.rollback();
}

inline QString lastErrorText(const QSqlError& err)
{
    return err.text();
}
}

#endif // DATABASEUTILS_H
