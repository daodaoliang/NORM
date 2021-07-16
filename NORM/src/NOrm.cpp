#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QThread>
#include <QStack>
#include "NOrm.h"

// 链接前缀
static const char *connectionPrefix = "_norm_";

// 对象映射
QMap<QByteArray, NOrmMetaModel> globalMetaModels = QMap<QByteArray, NOrmMetaModel>();

// 数据库对象
static NOrmDatabase *globalDatabase = nullptr;

// 数据库类型
static NOrmDatabase::DatabaseType globalDatabaseType = NOrmDatabase::UnknownDB;

// 调试模式
static bool globalDebugEnabled = false;

NOrmDatabase::NOrmDatabase(QObject *parent) : QObject(parent), connectionId(0)
{
}

void NOrmDatabase::threadFinished()
{
    QThread *thread = qobject_cast<QThread*>(sender());
    if (!thread)
        return;

    // 清理线程中的数据库链接
    QMutexLocker locker(&mutex);

    // 线程结束时 释放 数据库资源
    disconnect(thread, SIGNAL(finished()), this, SLOT(threadFinished()));

    // 获取数据库链接名字
    const QString connectionName = copies.value(thread).connectionName();

    // 移除映射表
    copies.remove(thread);

    // 数据库链接移除
    if (connectionName.startsWith(QLatin1String(connectionPrefix)))
        QSqlDatabase::removeDatabase(connectionName);
}

static void closeDatabase()
{
    delete globalDatabase;
    globalDatabase = nullptr;
}

static NOrmDatabase::DatabaseType getDatabaseType(QSqlDatabase &db)
{
    const QString driverName = db.driverName();
    if (driverName == QLatin1String("QMYSQL") || driverName == QLatin1String("QMYSQL3")) {
        return NOrmDatabase::MySqlServer;
    } else if (driverName == QLatin1String("QSQLITE") || driverName == QLatin1String("QSQLITE2")) {
        return NOrmDatabase::SQLite;
    } else if (driverName == QLatin1String("QPSQL")) {
        return NOrmDatabase::PostgreSQL;
    } else if (driverName == QLatin1String("QODBC")) {
        QSqlQuery query(db);
        if (query.exec("SELECT sqlite_version()"))
            return NOrmDatabase::SQLite;

        if (query.exec("SELECT @@version") && query.next() && query.value(0).toString().contains("Microsoft SQL Server"))
            return NOrmDatabase::MSSqlServer;

        if (query.exec("SELECT version()") && query.next()) {
            if (query.value(0).toString().contains("PostgreSQL")) {
                return NOrmDatabase::PostgreSQL;
            } else {
                return NOrmDatabase::MySqlServer;
            }
        }

        if (query.exec("SELECT * from v$version") && query.next()) {
            if (query.value(0).toString().contains("DM Database Server")) {
                return NOrmDatabase::DaMeng;
            }
        } else {
            if (globalDebugEnabled) {
                qWarning() << "SQL: " << query.executedQuery() << " SQL error: " <<  query.lastError();
            }
        }
    }
    return NOrmDatabase::UnknownDB;
}

static bool initDatabase(QSqlDatabase db)
{
    // 数据库的默认创建(目前仅支持 sqlite 和 mysql)
    NOrmDatabase::DatabaseType databaseType = NOrmDatabase::databaseType(db);

    // sqlite
    if (databaseType == NOrmDatabase::SQLite) {
        // 确保外见约束的处理
        NOrmQuery query(db);
        query.prepare("PRAGMA foreign_keys=on");
        query.exec();
    } else if (databaseType == NOrmDatabase::MySqlServer) {
        // create db if not exist
        auto dbName = QObject::tr("%1_tmp").arg(QString::number(quintptr(QThread::currentThreadId())));
        {
            QSqlDatabase tmpDB = QSqlDatabase::addDatabase("QMYSQL", dbName);
            tmpDB.setHostName(db.hostName());
            tmpDB.setPassword(db.password());
            tmpDB.setPort(db.port());
            tmpDB.setUserName(db.userName());
            if(!tmpDB.open()){
                qWarning()<<"db open error:"<<tmpDB.lastError().text();
                return false;
            }
            tmpDB.exec(QObject::tr("CREATE SCHEMA IF NOT EXISTS `%1`  DEFAULT CHARACTER SET utf8 ;").arg(db.databaseName()));
            tmpDB.exec(QObject::tr("use %1;").arg(db.databaseName()));
            tmpDB.close();
        }
    }
    return true;
}

NOrmQuery::NOrmQuery(QSqlDatabase db) : QSqlQuery(db)
{
    if (NOrmDatabase::databaseType(db) == NOrmDatabase::MSSqlServer) {
        // 设置前置游标
        setForwardOnly(true);
    }
}

void NOrmQuery::addBindValue(const QVariant &val, QSql::ParamType paramType)
{
    // this hack is required so that we do not store a mix of local and UTC times
    if (val.type() == QVariant::DateTime) {
        QSqlQuery::addBindValue(val.toDateTime().toLocalTime(), paramType);
    } else {
        QSqlQuery::addBindValue(val, paramType);
    }
}

bool NOrmQuery::exec()
{
    if (!QSqlQuery::exec()) {
        return false;
    }
    if (globalDebugEnabled)
        qWarning() << "SQL: " << executedQuery() << " SQL error: " <<  lastError();
    return true;
}

bool NOrmQuery::exec(const QString &query)
{
    if (!QSqlQuery::exec(query)) {
        return false;
    }
    if (globalDebugEnabled)
        qWarning() << "SQL: " << executedQuery() << " SQL error: " <<  lastError();
    return true;
}

QSqlDatabase NOrm::database()
{
    if (!globalDatabase)
        return QSqlDatabase();

    // 主线程返回主链接
    QThread *thread = QThread::currentThread();
    if (thread == globalDatabase->thread())
        return globalDatabase->reference;

    // 该链接已经创建过则直接返回
    QMutexLocker locker(&globalDatabase->mutex);
    if (globalDatabase->copies.contains(thread))
        return globalDatabase->copies[thread];

    // 新的线程则创建一个新的数据库链接
    QObject::connect(thread, SIGNAL(finished()), globalDatabase, SLOT(threadFinished()));
    QSqlDatabase db = QSqlDatabase::cloneDatabase(globalDatabase->reference, QLatin1String(connectionPrefix) + QString::number(globalDatabase->connectionId++));
    if(db.open()){
        initDatabase(db);
        if(globalDebugEnabled){
            qDebug() << "线程:" << QThread::currentThread() << "创建了一个新的数据库链接 " << globalDatabase->connectionId;
        }
    } else {
        qWarning() << "线程:" << QThread::currentThread() << "创建了一个新的数据库链接失败 " << globalDatabase->connectionId;
    }
    globalDatabase->copies.insert(thread, db);
    return db;
}

bool NOrm::setDatabase(QSqlDatabase database)
{
    // 数据库确保打开
    bool ret_openDB = database.isOpen();
    if (!ret_openDB){
        ret_openDB = database.open();
        if (!ret_openDB) {
            return false;
        }
    }
    globalDatabaseType = getDatabaseType(database);
    if (globalDatabaseType == NOrmDatabase::UnknownDB) {
        qWarning() << "Unsupported database driver" << database.driverName();
    }

    if (!globalDatabase) {
        globalDatabase = new NOrmDatabase();
        qAddPostRoutine(closeDatabase);
    }

    // 初始化数据库
    bool ret = initDatabase(database);

    globalDatabase->reference = database;
    return ret && ret_openDB;
}

bool NOrm::isDebugEnabled()
{
    return globalDebugEnabled;
}

void NOrm::setDebugEnabled(bool enabled)
{
    globalDebugEnabled = enabled;
}

static void norm_topsort(const QByteArray &modelName,
                         QHash<QByteArray, bool> &visited,
                         QStack<NOrmMetaModel> &stack)
{
    visited[modelName] = true;
    NOrmMetaModel model = globalMetaModels[modelName];
    foreach (const QByteArray &foreignModel, model.foreignFields().values()) {
        if (!visited[foreignModel])
            norm_topsort(foreignModel, visited, stack);
    }

    stack.push(model);
}

static QStack<NOrmMetaModel> norm_sorted_metamodels()
{
    QStack<NOrmMetaModel> stack;
    stack.reserve(globalMetaModels.size());
    QHash<QByteArray, bool> visited;
    visited.reserve(globalMetaModels.size());
    foreach (const QByteArray &model, globalMetaModels.keys())
        visited[model] = false;

    foreach (const QByteArray &model, globalMetaModels.keys()) {
        if (!visited[model])
            norm_topsort(model, visited, stack);
    }

    return stack;
}

bool NOrm::createTables()
{
    bool result = true;
    QStack<NOrmMetaModel> stack = norm_sorted_metamodels();
    foreach (const NOrmMetaModel &model, stack) {
        if (!model.createTable())
            result = false;
    }

    return result;
}

bool NOrm::dropTables()
{
    bool result = true;
    QStack<NOrmMetaModel> stack = norm_sorted_metamodels();
    for (int i = stack.size() - 1; i >= 0; --i) {
        NOrmMetaModel model = stack.at(i);
        if (!model.dropTable())
            result = false;
    }

    return result;
}

NOrmMetaModel NOrm::metaModel(const char *name)
{
    if (globalMetaModels.contains(name))
        return globalMetaModels.value(name);

    // 直接去全局做名字匹配
    foreach (QByteArray modelName, globalMetaModels.keys()) {
        if (qstricmp(name, modelName.data()) == 0)
            return globalMetaModels.value(modelName);
    }

    return NOrmMetaModel();
}

QStack<NOrmMetaModel> NOrm::metaModels()
{
    return norm_sorted_metamodels();
}

NOrmMetaModel NOrm::registerModel(const QMetaObject *meta)
{
    const QByteArray name = meta->className();
    if (!globalMetaModels.contains(name))
        globalMetaModels.insert(name, NOrmMetaModel(meta));
    return globalMetaModels[name];
}

NOrmDatabase::DatabaseType NOrmDatabase::databaseType(const QSqlDatabase &db)
{
    Q_UNUSED(db);
    return globalDatabaseType;
}
