#ifndef NORM_P_H
#define NORM_P_H

#include <QMap>
#include <QMutex>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

/**
 * @brief The NOrmDatabase class
 * 链接数据库的操作集合
 */
class NOrmDatabase : public QObject
{
    Q_OBJECT

public:
    NOrmDatabase(QObject *parent = 0);

    /**
     * @brief The DatabaseType enum 数据库类型
     */
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

    /**
     * @brief databaseType 获取数据库类型
     * @param db 数据库信息
     * @return 数据库类型
     */
    static DatabaseType databaseType(const QSqlDatabase &db);

    QSqlDatabase reference;
    QMutex mutex;
    QMap<QThread*, QSqlDatabase> copies;
    qint64 connectionId;

private slots:
    void threadFinished();
};

class NOrmQuery : public QSqlQuery
{
public:
    NOrmQuery(QSqlDatabase db);
    void addBindValue(const QVariant &val, QSql::ParamType paramType = QSql::In);
    bool exec();
    bool exec(const QString &query);
};

#endif
