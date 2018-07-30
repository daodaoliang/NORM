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

static const char *connectionPrefix = "_norm_";

QMap<QByteArray, NOrmMetaModel> globalMetaModels = QMap<QByteArray, NOrmMetaModel>();
static NOrmDatabase *globalDatabase = 0;
static NOrmDatabase::DatabaseType globalDatabaseType = NOrmDatabase::UnknownDB;
static bool globalDebugEnabled = false;

NOrmDatabase::NOrmDatabase(QObject *parent)
    : QObject(parent), connectionId(0)
{
}

void NOrmDatabase::threadFinished()
{
    QThread *thread = qobject_cast<QThread*>(sender());
    if (!thread)
        return;

    // cleanup database connection for the thread
    QMutexLocker locker(&mutex);
    disconnect(thread, SIGNAL(finished()), this, SLOT(threadFinished()));
    const QString connectionName = copies.value(thread).connectionName();
    copies.remove(thread);
    if (connectionName.startsWith(QLatin1String(connectionPrefix)))
        QSqlDatabase::removeDatabase(connectionName);
}

static void closeDatabase()
{
    delete globalDatabase;
    globalDatabase = 0;
}

static NOrmDatabase::DatabaseType getDatabaseType(QSqlDatabase &db)
{
    const QString driverName = db.driverName();
    if (driverName == QLatin1String("QMYSQL") ||
        driverName == QLatin1String("QMYSQL3"))
        return NOrmDatabase::MySqlServer;
    else if (driverName == QLatin1String("QSQLITE") ||
             driverName == QLatin1String("QSQLITE2"))
        return NOrmDatabase::SQLite;
    else if (driverName == QLatin1String("QPSQL"))
        return NOrmDatabase::PostgreSQL;
    else if (driverName == QLatin1String("QODBC")) {
        QSqlQuery query(db);
        if (query.exec("SELECT sqlite_version()"))
            return NOrmDatabase::SQLite;

        if (query.exec("SELECT @@version") && query.next() &&
            query.value(0).toString().contains("Microsoft SQL Server"))
                return NOrmDatabase::MSSqlServer;

        if (query.exec("SELECT version()") && query.next()) {
            if (query.value(0).toString().contains("PostgreSQL"))
                return NOrmDatabase::PostgreSQL;
            else
                return NOrmDatabase::MySqlServer;
        }
    }
    return NOrmDatabase::UnknownDB;
}

static bool initDatabase(QSqlDatabase db)
{
    NOrmDatabase::DatabaseType databaseType = NOrmDatabase::databaseType(db);
    if (databaseType == NOrmDatabase::SQLite) {
        // enable foreign key constraint handling
        NOrmQuery query(db);
        query.prepare("PRAGMA foreign_keys=on");
        query.exec();
    } else if (databaseType == NOrmDatabase::MySqlServer) {
        // create db if not exist
        QSqlDatabase tmpDB = QSqlDatabase::addDatabase("QMYSQL","tmp_connection");
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
//        QSqlDatabase::removeDatabase("tmp_connection");
    }
    return true;
}

NOrmQuery::NOrmQuery(QSqlDatabase db)
    : QSqlQuery(db)
{
    if (NOrmDatabase::databaseType(db) == NOrmDatabase::MSSqlServer) {
        // default to fast-forward cursor
        setForwardOnly(true);
    }
}

void NOrmQuery::addBindValue(const QVariant &val, QSql::ParamType paramType)
{
    // this hack is required so that we do not store a mix of local
    // and UTC times
    if (val.type() == QVariant::DateTime)
        QSqlQuery::addBindValue(val.toDateTime().toLocalTime(), paramType);
    else
        QSqlQuery::addBindValue(val, paramType);
}

bool NOrmQuery::exec()
{
    if (globalDebugEnabled) {
        qDebug() << "SQL query" << lastQuery();
        QMapIterator<QString, QVariant> i(boundValues());
        while (i.hasNext()) {
            i.next();
            qDebug() << "SQL   " << i.key().toLatin1().data() << "="
                     << i.value().toString().toLatin1().data();
        }
    }
    if (!QSqlQuery::exec()) {
        if (globalDebugEnabled)
            qWarning() << "SQL error" << lastError();
        return false;
    }
    return true;
}

bool NOrmQuery::exec(const QString &query)
{
    if (globalDebugEnabled)
        qDebug() << "SQL query" << query;
    if (!QSqlQuery::exec(query)) {
        if (globalDebugEnabled)
            qWarning() << "SQL error" << lastError();
        return false;
    }
    return true;
}

QSqlDatabase NOrm::database()
{
    if (!globalDatabase)
        return QSqlDatabase();

    // if we are in the main thread, return reference connection
    QThread *thread = QThread::currentThread();
    if (thread == globalDatabase->thread())
        return globalDatabase->reference;

    // if we have a connection for this thread, return it
    QMutexLocker locker(&globalDatabase->mutex);
    if (globalDatabase->copies.contains(thread))
        return globalDatabase->copies[thread];

    // create a new connection for this thread
    QObject::connect(thread, SIGNAL(finished()), globalDatabase, SLOT(threadFinished()));
    QSqlDatabase db = QSqlDatabase::cloneDatabase(globalDatabase->reference,
        QLatin1String(connectionPrefix) + QString::number(globalDatabase->connectionId++));
    db.open();
    initDatabase(db);
    globalDatabase->copies.insert(thread, db);
    return db;
}

bool NOrm::setDatabase(QSqlDatabase database)
{
    globalDatabaseType = getDatabaseType(database);
    if (globalDatabaseType == NOrmDatabase::UnknownDB) {
        qWarning() << "Unsupported database driver" << database.driverName();
    }

    if (!globalDatabase) {
        globalDatabase = new NOrmDatabase();
        qAddPostRoutine(closeDatabase);
    }
    bool ret = initDatabase(database);
    bool ret_openDB = database.open();
    globalDatabase->reference = database;
    return ret && ret_openDB;
}

/*!
    Returns whether debugging information should be printed.

    \sa setDebugEnabled()
*/
bool NOrm::isDebugEnabled()
{
    return globalDebugEnabled;
}

/*!
    Sets whether debugging information should be printed.

    \sa isDebugEnabled()
*/
void NOrm::setDebugEnabled(bool enabled)
{
    globalDebugEnabled = enabled;
}

static void norm_topsort(const QByteArray &modelName, QHash<QByteArray, bool> &visited,
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

/*!
    Creates the database tables for all registered models.

    \return true if all the tables were created, false otherwise.
*/
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

    // otherwise, try to find a model anyway
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
