#ifndef NORM_P_H
#define NORM_P_H

/*
 * 描述: NORM 私有接口
 * 作者: daodaoliang@yeah.net
 * 时间: 2021-07-16
 */

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
    NOrmDatabase(QObject *parent = nullptr);

    /**
     * @brief The DatabaseType enum 数据库类型
     */
    enum DatabaseType {
        // 未知数据库类型
        UnknownDB,
        // sqlserver
        MSSqlServer,
        // mysql
        MySqlServer,
        // pgsql
        PostgreSQL,
        // oracle
        Oracle,
        // sybase
        Sybase,
        // sqlite
        SQLite,
        // interbase
        Interbase,
        // db2
        DB2,
        // 达梦
        DaMeng
    };

    /**
     * @brief databaseType 获取数据库类型
     * @param db 数据库信息
     * @return 数据库类型
     */
    static DatabaseType databaseType(const QSqlDatabase &db);

    // 数据库对象
    QSqlDatabase reference;

    // 数据库锁
    QMutex mutex;

    // 线程 和 数据库链接的映射
    QMap<QThread*, QSqlDatabase> copies;

    // 链接ID
    qint64 connectionId;

private slots:

    // 线程结束
    void threadFinished();
};

/**
 * @brief The NOrmQuery class 数据库查询对象
 */
class NOrmQuery : public QSqlQuery
{
public:
    NOrmQuery(QSqlDatabase db);
    void addBindValue(const QVariant &val, QSql::ParamType paramType = QSql::In);
    bool exec();
    bool exec(const QString &query);
};

#endif
