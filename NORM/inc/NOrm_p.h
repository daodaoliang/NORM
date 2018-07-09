#ifndef NORM_P_H
#define NORM_P_H

#include <QMap>
#include <QMutex>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include "NOrm_global.h"

/** \brief The NOrmDatabase class represents a set of connections to a
 *  database.
 *
 * \internal
 */
class NORMSHARED_EXPORT NOrmDatabase : public QObject
{
    Q_OBJECT

public:
    NOrmDatabase(QObject *parent = 0);

    enum DatabaseType {
        UnknownDB,
        MSSqlServer,
        MySqlServer,
        PostgreSQL,
        Oracle,
        Sybase,
        SQLite,
        Interbase,
        DB2
    };

    static DatabaseType databaseType(const QSqlDatabase &db);

    QSqlDatabase reference;
    QMutex mutex;
    QMap<QThread*, QSqlDatabase> copies;
    qint64 connectionId;

private slots:
    void threadFinished();
};

class NORMSHARED_EXPORT NOrmQuery : public QSqlQuery
{
public:
    NOrmQuery(QSqlDatabase db);
    void addBindValue(const QVariant &val, QSql::ParamType paramType = QSql::In);
    bool exec();
    bool exec(const QString &query);
};

#endif
