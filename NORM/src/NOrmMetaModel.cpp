#include <QDebug>
#include <QMetaProperty>
#include <QSqlDriver>
#include <QStringList>
#include "NOrm.h"
#include "NOrmMetaModel.h"
#include "NOrmQuerySet_p.h"

// python-compatible hash
static long string_hash(const QString &s)
{
    if (s.isEmpty())
        return 0;

    const QByteArray a = s.toLatin1();
    unsigned char *p = (unsigned char *) a.constData();
    long x = *p << 7;
    for (int i = 0; i < a.size(); ++i)
        x = (1000003*x) ^ *p++;
    x ^= a.size();
    return (x == -1) ? -2 : x;
}

static long stringlist_hash(const QStringList &l)
{
    long x = 0x345678L;
    long mult = 1000003L;
    int len = l.size();
    foreach (const QString &s, l) {
        --len;
        x = (x ^ string_hash(s)) * mult;
        mult += (long)(82520L + len + len);
    }
    x += 97531L;
    return (x == -1) ? -2 : x;
}

static QString stringlist_digest(const QStringList &l)
{
    return QString::number(labs(stringlist_hash(l)) % 4294967296UL, 16);
}

enum ForeignKeyConstraint
{
    NoAction,
    Restrict,
    Cascade,
    SetNull
};

// 数据表对象字段私有类
class NOrmMetaFieldPrivate : public QSharedData
{
public:
    NOrmMetaFieldPrivate();

    // 是否自增索引
    bool autoIncrement;

    // 列名字
    QString db_column;

    // 外键管理模型名字
    QByteArray foreignModel;

    // 是否是索引字段
    bool index;

    // 最大长度(字符串类型)
    int maxLength;

    // 模型列名字
    QByteArray name;

    // 是否允许null
    bool null;

    // 数据字段类型
    QVariant::Type type;

    // 是否是唯一性约束
    bool unique;

    // 是否是空
    bool blank;

    // 是否含有删除约束
    ForeignKeyConstraint deleteConstraint;
};

NOrmMetaFieldPrivate::NOrmMetaFieldPrivate()
    : autoIncrement(false)
    , index(false)
    , maxLength(0)
    , null(false)
    , unique(false)
    , blank(false)
    , deleteConstraint(NoAction)
{
}

NOrmMetaField::NOrmMetaField()
{
    d = new NOrmMetaFieldPrivate;
}

NOrmMetaField::NOrmMetaField(const NOrmMetaField &other)
    : d(other.d)
{
}

NOrmMetaField::~NOrmMetaField()
{
}

NOrmMetaField& NOrmMetaField::operator=(const NOrmMetaField& other)
{
    d = other.d;
    return *this;
}

QString NOrmMetaField::column() const
{
    return d->db_column;
}

bool NOrmMetaField::isNullable() const
{
    return d->null;
}

bool NOrmMetaField::isValid() const
{
    return !d->name.isEmpty();
}

bool NOrmMetaField::isAutoIncrement() const
{
    return d->autoIncrement;
}

bool NOrmMetaField::isUnique() const
{
    return d->unique;
}

bool NOrmMetaField::isBlank() const
{
    return d->blank;
}

QString NOrmMetaField::name() const
{
    return QString::fromLatin1(d->name);
}

int NOrmMetaField::maxLength() const
{
    return d->maxLength;
}

QVariant NOrmMetaField::toDatabase(const QVariant &value) const
{
    if (d->type == QVariant::String && !d->null && value.isNull()){
        return QLatin1String("");
    } else if (!d->foreignModel.isEmpty() && d->type == QVariant::Int && d->null && !value.toInt()) {
        return QVariant();
    } else if (d->type == QVariant::StringList) {
        QStringList tmpStr = value.value<QStringList>();
        QString arrary_data;
        foreach (QString item, tmpStr) {
            arrary_data += QObject::tr("%1,").arg(item);
        }
        arrary_data = arrary_data.left(arrary_data.length()-1);
        if(arrary_data.isNull()){
            arrary_data = "";
        }
        QVariant tmpValue(arrary_data);
        return tmpValue;
    } else {
        return value;
    }
}

static QMap<QString, QString> parseOptions(const char *value)
{
    QMap<QString, QString> options;
    QStringList items = QString::fromLatin1(value).split(QLatin1Char(' '), QString::SkipEmptyParts);
    foreach (const QString &item, items) {
        QStringList assign = item.split(QLatin1Char('='));
        if (assign.size() == 2) {
            options[assign[0].toLower()] = assign[1];
        } else {
            qWarning() << "Could not parse option" << item;
        }
    }
    return options;
}

static bool stringToBool(const QString &value)
{
    return value.toLower() == QLatin1String("true") || value == QLatin1String("1");
}

// 数据表元模型似有类
class NOrmMetaModelPrivate : public QSharedData
{
public:
    // 数据表名字
    QString className;

    // 包含的列信息
    QList<NOrmMetaField> localFields;

    // 外键列信息
    QMap<QByteArray, QByteArray> foreignFields;

    // 主键列表
    QByteArray primaryKey;

    // 表名字
    QString table;

    // 唯一性字段
    QList<QByteArray> uniqueTogether;
};

NOrmMetaModel::NOrmMetaModel(const QMetaObject *meta) : d(new NOrmMetaModelPrivate)
{
    if (!meta)
        return;

    // 类名字映射
    d->className = meta->className();

    // 表名字映射
    d->table = QString::fromLatin1(meta->className()).toLower();

    // 解析数据表的操作项
    const int optionsIndex = meta->indexOfClassInfo("__meta__");
    if (optionsIndex >= 0) {
        QMap<QString, QString> options = parseOptions(meta->classInfo(optionsIndex).value());
        QMapIterator<QString, QString> option(options);
        while (option.hasNext()) {
            option.next();
            if (option.key() == QLatin1String("db_table")) {
                d->table = option.value();
            } else if (option.key() == QLatin1String("unique_together")) {
                d->uniqueTogether = option.value().toLatin1().split(',');
            }
        }
    }

    // 逐个属性解析
    const int count = meta->propertyCount();
    for(int i = QObject::staticMetaObject.propertyCount(); i < count; ++i)
    {
        const QString typeName = QString::fromLatin1(meta->property(i).typeName());
        if (!qstrcmp(meta->property(i).name(), "pk"))
            continue;

        // 解析字段属性
        bool autoIncrementOption = false;
        QString dbColumnOption;
        bool dbIndexOption = false;
        bool ignoreFieldOption = false;
        int maxLengthOption = 0;
        bool primaryKeyOption = false;
        bool nullOption = false;
        bool uniqueOption = false;
        bool blankOption = false;
        ForeignKeyConstraint deleteConstraint = NoAction;
        const int infoIndex = meta->indexOfClassInfo(meta->property(i).name());
        if (infoIndex >= 0)
        {
            // 属性映射
            QMap<QString, QString> options = parseOptions(meta->classInfo(infoIndex).value());
            QMapIterator<QString, QString> option(options);
            while (option.hasNext()) {
                option.next();
                const QString key = option.key();
                const QString value = option.value();
                if (key == QLatin1String("auto_increment")) {
                    autoIncrementOption = stringToBool(value);
                } else if (key == QLatin1String("db_column")) {
                    dbColumnOption = value;
                } else if (key == QLatin1String("db_index")) {
                    dbIndexOption = stringToBool(value);
                } else if (key == QLatin1String("ignore_field")) {
                    ignoreFieldOption = stringToBool(value);
                } else if (key == QLatin1String("max_length")) {
                    maxLengthOption = value.toInt();
                } else if (key == QLatin1String("null")) {
                    nullOption = stringToBool(value);
                } else if (key == QLatin1String("primary_key")) {
                    primaryKeyOption = stringToBool(value);
                } else if (key == QLatin1String("unique")) {
                    uniqueOption = stringToBool(value);
                } else if (key == QLatin1String("blank")) {
                    blankOption = stringToBool(value);
                } else if (option.key() == "on_delete") {
                    if (value.toLower() == "cascade") {
                        deleteConstraint = Cascade;
                    } else if (value.toLower() == "set_null") {
                        deleteConstraint = SetNull;
                    } else if (value.toLower() == "restrict") {
                        deleteConstraint = Restrict;
                    }
                }
            }
        }

        // 忽略字段
        if (ignoreFieldOption)
            continue;

        // 外键字段
        if (typeName.endsWith(QLatin1Char('*'))) {
            const QByteArray fkName = meta->property(i).name();
            const QByteArray fkModel = typeName.left(typeName.size() - 1).toLatin1();
            d->foreignFields.insert(fkName, fkModel);

            NOrmMetaField field;
            field.d->name = fkName + "_id";
            field.d->type = QVariant::Int;
            field.d->foreignModel = fkModel;
            field.d->db_column = dbColumnOption.isEmpty() ? QString::fromLatin1(field.d->name) : dbColumnOption;
            field.d->index = true;
            field.d->null = nullOption;
            field.d->deleteConstraint = deleteConstraint;
            d->localFields << field;
            continue;
        }

        // 表字段
        NOrmMetaField field;
        field.d->name = meta->property(i).name();
        field.d->type = meta->property(i).type();
        field.d->db_column = dbColumnOption.isEmpty() ? QString::fromLatin1(field.d->name) : dbColumnOption;
        field.d->maxLength = maxLengthOption;
        field.d->null = nullOption;
        if (primaryKeyOption) {
            field.d->autoIncrement = autoIncrementOption;
            d->primaryKey = field.d->name;
        } else if (uniqueOption) {
            field.d->unique = true;
        } else if (blankOption) {
            field.d->blank = true;
        } else if (dbIndexOption) {
            field.d->index = true;
        }

        d->localFields << field;
    }

    // 自增主键
    if (d->primaryKey.isEmpty()) {
        NOrmMetaField field;
        field.d->name = "id";
        field.d->type = QVariant::Int;
        field.d->db_column = QLatin1String("id");
        field.d->null = false;
        field.d->autoIncrement = true;
        d->localFields.prepend(field);
        d->primaryKey = field.d->name;
    }

}

NOrmMetaModel::NOrmMetaModel(const NOrmMetaModel &other) : d(other.d)
{
}

NOrmMetaModel::~NOrmMetaModel()
{
}

QString NOrmMetaModel::className() const
{
    return d->className;
}

bool NOrmMetaModel::isValid() const
{
    return !d->table.isNull();
}

NOrmMetaModel& NOrmMetaModel::operator=(const NOrmMetaModel& other)
{
    d = other.d;
    return *this;
}

bool NOrmMetaModel::createTable() const
{
    NOrmQuery createQuery(NOrm::database());
    foreach (const QString &sql, createTableSql()) {
        if (!createQuery.exec(sql))
            return false;
    }
    return true;
}

QStringList NOrmMetaModel::createTableSql() const
{
    QSqlDatabase db = NOrm::database();
    QSqlDriver *driver = db.driver();
    NOrmDatabase::DatabaseType databaseType = NOrmDatabase::databaseType(db);

    QStringList queries;
    QStringList propSql;
    QStringList constraintSql;
    const QString quotedTable = db.driver()->escapeIdentifier(d->table, QSqlDriver::TableName);
    foreach (const NOrmMetaField &field, d->localFields)
    {
        QString fieldSql = driver->escapeIdentifier(field.column(), QSqlDriver::FieldName);
        fieldSql += " ";
        switch (field.d->type) {
        case QVariant::Bool:
            fieldSql += getBoolType(databaseType);
            break;
        case QVariant::ByteArray:
            fieldSql += getByteArrayType(databaseType, field.d->maxLength);
            break;
        case QVariant::Date:
            fieldSql += getDateType(databaseType);
            break;
        case QVariant::DateTime:
            fieldSql += getDateTimeType(databaseType);
            break;
        case QVariant::Double:
            fieldSql += getDoubleType(databaseType);
            break;
        case QVariant::Int:
            fieldSql += getIntType(databaseType);
            break;
        case QVariant::LongLong:
            fieldSql += getLongLongType(databaseType);
            break;
        case QVariant::String:
            fieldSql += getStringType(databaseType, field.d->maxLength);
            break;
        case QVariant::Time:
            fieldSql += getTimeType(databaseType);
            break;
        case QVariant::StringList:
            fieldSql += getStringListType(databaseType, field.d->maxLength);
            break;
        default:
            qWarning() << "Unhandled type" << field.d->type << "for property" << field.d->name;
            continue;
        }

        if (field.d->null) {
            fieldSql += " ";
            fieldSql += attributeNull(databaseType);
        }

        if (field.d->unique) {
            fieldSql += " ";
            fieldSql += attributeUnique(databaseType);
        }

        // primary key
        if (field.d->name == d->primaryKey) {
            fieldSql += " ";
            fieldSql += attributePrimaryKey(databaseType);
        }

        // auto-increment is backend specific
        if (field.d->autoIncrement) {
            fieldSql += " ";
            fieldSql += attributeAutoIncrement(databaseType,field);
        }

        // foreign key
        if (!field.d->foreignModel.isEmpty())
        {
            const NOrmMetaModel foreignMeta = NOrm::metaModel(field.d->foreignModel);
            const NOrmMetaField foreignField = foreignMeta.localField("pk");
            if (databaseType == NOrmDatabase::MySqlServer) {
                QString constraintName = QString::fromLatin1("FK_%1_%2").arg(
                            field.column(), stringlist_digest(QStringList() << field.column() << d->table));
                QString constraint =
                        QString::fromLatin1("CONSTRAINT %1 FOREIGN KEY (%2) REFERENCES %3 (%4)").arg(
                            driver->escapeIdentifier(constraintName, QSqlDriver::FieldName),
                            driver->escapeIdentifier(field.column(), QSqlDriver::FieldName),
                            driver->escapeIdentifier(foreignMeta.d->table, QSqlDriver::TableName),
                            driver->escapeIdentifier(foreignField.column(), QSqlDriver::FieldName)
                            );

                if (field.d->deleteConstraint != NoAction) {
                    constraint += " ON DELETE";
                    switch (field.d->deleteConstraint) {
                    case Cascade:
                        constraint += " CASCADE";
                        break;
                    case SetNull:
                        constraint += " SET NULL";
                        break;
                    case Restrict:
                        constraint += " RESTRICT";
                        break;
                    default:
                        break;
                    }
                }
                constraintSql << constraint;
            } else {
                fieldSql += QString::fromLatin1(" REFERENCES %1 (%2)").arg(
                            driver->escapeIdentifier(foreignMeta.d->table, QSqlDriver::TableName),
                            driver->escapeIdentifier(foreignField.column(), QSqlDriver::FieldName));

                if (databaseType == NOrmDatabase::MSSqlServer &&
                        field.d->deleteConstraint == Restrict) {
                    qWarning("MSSQL does not support RESTRICT constraints");
                    break;
                }

                if (field.d->deleteConstraint != NoAction) {
                    fieldSql += " ON DELETE";
                    switch (field.d->deleteConstraint) {
                    case Cascade:
                        fieldSql += " CASCADE";
                        break;
                    case SetNull:
                        fieldSql += " SET NULL";
                        break;
                    case Restrict:
                        fieldSql += " RESTRICT";
                        break;
                    default:
                        break;
                    }
                }
            }

            if (databaseType == NOrmDatabase::PostgreSQL)
                fieldSql += " DEFERRABLE INITIALLY DEFERRED";
        }
        propSql << fieldSql;
    }

    // add constraints if we need them
    if (!constraintSql.isEmpty())
        propSql << constraintSql.join(QLatin1String(", "));

    // unique contraints
    if (!d->uniqueTogether.isEmpty()) {
        QStringList columns;
        foreach (const QByteArray &name, d->uniqueTogether) {
            columns << driver->escapeIdentifier(localField(name).column(), QSqlDriver::FieldName);
        }
        propSql << QString::fromLatin1("UNIQUE (%2)").arg(columns.join(QLatin1String(", ")));
    }

    // create table
    queries << QString::fromLatin1("CREATE TABLE %1 (%2)").arg(
                   quotedTable,
                   propSql.join(QLatin1String(", ")));

    // create indices
    foreach (const NOrmMetaField &field, d->localFields) {
        if (field.d->index) {
            const QString indexName = d->table + QLatin1Char('_')
                    + stringlist_digest(QStringList() << field.column());
            queries << QString::fromLatin1("CREATE INDEX %1 ON %2 (%3)").arg(
                           // FIXME : how should we escape an index name?
                           driver->escapeIdentifier(indexName, QSqlDriver::FieldName),
                           quotedTable,
                           driver->escapeIdentifier(field.column(), QSqlDriver::FieldName));
        }
    }

    return queries;
}

bool NOrmMetaModel::dropTable() const
{
    QSqlDatabase db = NOrm::database();
    if (!db.tables().contains(d->table))
        return true;

    NOrmQuery query(db);
    return query.exec(QLatin1String("DROP TABLE ") +
                      db.driver()->escapeIdentifier(d->table, QSqlDriver::TableName));
}

QObject *NOrmMetaModel::foreignKey(const QObject *model, const char *name) const
{
    // check the name is valid
    const QByteArray prop(name);
    if (!d->foreignFields.contains(prop)) {
        qWarning("NOrmMetaModel cannot get foreign model for invalid key '%s'", name);
        return nullptr;
    }

    QObject *foreign = model->property(prop + "_ptr").value<QObject*>();
    if (!foreign)
        return nullptr;

    // if the foreign object was not loaded yet, do it now
    const QByteArray foreignClass = d->foreignFields[prop];
    const NOrmMetaModel foreignMeta = NOrm::metaModel(foreignClass);
    const QVariant foreignPk = model->property(prop + "_id");
    if (foreign->property(foreignMeta.primaryKey()) != foreignPk)
    {
        NOrmQuerySetPrivate qs(foreignClass);
        qs.addFilter(NOrmWhere(QLatin1String("pk"), NOrmWhere::Equals, foreignPk));
        qs.sqlFetch();
        if (qs.properties.size() != 1 || !qs.sqlLoad(foreign, 0))
            return nullptr;
    }
    return foreign;
}

void NOrmMetaModel::setForeignKey(QObject *model, const char *name, QObject *value) const
{
    // check the name is valid
    const QByteArray prop(name);
    if (!d->foreignFields.contains(prop)) {
        qWarning("NOrmMetaModel cannot set foreign model for invalid key '%s'", name);
        return;
    }

    QObject *old = model->property(prop + "_ptr").value<QObject*>();
    if (old == value)
        return;

    // store the new pointer and update the foreign key
    model->setProperty(prop + "_ptr", qVariantFromValue(value));
    if (value) {
        const NOrmMetaModel foreignMeta = NOrm::metaModel(d->foreignFields[prop]);
        model->setProperty(prop + "_id", value->property(foreignMeta.primaryKey()));
    } else {
        model->setProperty(prop + "_id", QVariant());
    }
}

void NOrmMetaModel::load(QObject *model, const QVariantList &properties, int &pos, const QStringList &relatedFields) const
{
    // process local fields
    foreach (const NOrmMetaField &field, d->localFields){

        if(field.d->type == QVariant::StringList) {
            QVariant tmpObj;
            QStringList tmpStrList = properties.at(pos++).toStringList();
            if(tmpStrList.size() > 0){
                QString tmpStr = tmpStrList.first();
                tmpObj = QVariant::fromValue(tmpStr.split(','));
            } else {
                tmpObj = QVariant::fromValue(QString(""));
            }
            model->setProperty(field.d->name, tmpObj);
        } else {
            model->setProperty(field.d->name, properties.at(pos++));
        }
    }

    // process foreign fields
    if (pos >= properties.size())
        return;
    foreach (const QByteArray &fkName, d->foreignFields.keys())
    {
        QString fkS(fkName);
        if ( relatedFields.contains(fkS) )
        {
            QStringList nsl = relatedFields.filter(QRegExp("^" + fkS + "__")).replaceInStrings(QRegExp("^" + fkS + "__"),"");
            QObject *object = model->property(fkName + "_ptr").value<QObject*>();
            if (object)
            {
                const NOrmMetaModel foreignMeta = NOrm::metaModel(d->foreignFields[fkName]);
                foreignMeta.load(object, properties, pos, nsl);
            }
        }

        if (relatedFields.isEmpty())
        {
            QObject *object = model->property(fkName + "_ptr").value<QObject*>();
            if (object)
            {
                const NOrmMetaModel foreignMeta = NOrm::metaModel(d->foreignFields[fkName]);
                foreignMeta.load(object, properties, pos);
            }
        }
    }
}

/*!
    Returns the foreign field mapping.
*/
QMap<QByteArray, QByteArray> NOrmMetaModel::foreignFields() const
{
    return d->foreignFields;
}

/*!
    Return the local field with the specified \a name.
*/
NOrmMetaField NOrmMetaModel::localField(const char *name) const
{
    const QByteArray fieldName = strcmp(name, "pk") ? QByteArray(name) : d->primaryKey;
    foreach (const NOrmMetaField &field, d->localFields) {
        if (field.d->name == fieldName)
            return field;
    }
    return NOrmMetaField();
}

QList<NOrmMetaField> NOrmMetaModel::localFields() const
{
    return d->localFields;
}

QByteArray NOrmMetaModel::primaryKey() const
{
    return d->primaryKey;
}

QString NOrmMetaModel::table() const
{
    return d->table;
}

QString NOrmMetaModel::getBoolType(NOrmDatabase::DatabaseType databaseType) const
{
    switch (databaseType) {
    case NOrmDatabase::UnknownDB:
    case NOrmDatabase::MySqlServer:
    case NOrmDatabase::Oracle:
    case NOrmDatabase::Sybase:
    case NOrmDatabase::SQLite:
    case NOrmDatabase::Interbase:
    case NOrmDatabase::DB2:
        return  QString("bool");
    case NOrmDatabase::PostgreSQL:
        return  QString("boolean");
    case NOrmDatabase::MSSqlServer:
    case NOrmDatabase::DaMeng:
        return QString("bit");
    }
    return QString();
}

QString NOrmMetaModel::getByteArrayType(NOrmDatabase::DatabaseType databaseType, int maxLength) const
{
    QString sql;
    switch (databaseType) {
    case NOrmDatabase::UnknownDB:
    case NOrmDatabase::MySqlServer:
    case NOrmDatabase::Oracle:
    case NOrmDatabase::Sybase:
    case NOrmDatabase::SQLite:
    case NOrmDatabase::Interbase:
    case NOrmDatabase::DB2:
    case NOrmDatabase::DaMeng:
        sql += QLatin1String(" blob");
        if (maxLength > 0) {
            sql += QLatin1Char('(') + QString::number(maxLength) + QLatin1Char(')');
        }
        break;
    case NOrmDatabase::MSSqlServer:
        sql += QLatin1String("varbinary");
        if (maxLength > 0) {
            sql += QLatin1Char('(') + QString::number(maxLength) + QLatin1Char(')');
        }
        else {
            sql += QLatin1String("(max)");
        }
        break;
    case NOrmDatabase::PostgreSQL:
        sql += QLatin1String("bytea");
        break;
    }
    return sql;
}

QString NOrmMetaModel::getDateType(NOrmDatabase::DatabaseType databaseType) const
{
    switch (databaseType) {
    case NOrmDatabase::UnknownDB:
    case NOrmDatabase::MSSqlServer:
    case NOrmDatabase::MySqlServer:
    case NOrmDatabase::PostgreSQL:
    case NOrmDatabase::Oracle:
    case NOrmDatabase::Sybase:
    case NOrmDatabase::SQLite:
    case NOrmDatabase::Interbase:
    case NOrmDatabase::DB2:
    case NOrmDatabase::DaMeng:
        return QString("date");
    }
    return QString();
}

QString NOrmMetaModel::getDateTimeType(NOrmDatabase::DatabaseType databaseType) const
{
    QString fieldSql;
    switch (databaseType) {
    case NOrmDatabase::UnknownDB:
    case NOrmDatabase::MSSqlServer:
    case NOrmDatabase::MySqlServer:
    case NOrmDatabase::Oracle:
    case NOrmDatabase::Sybase:
    case NOrmDatabase::SQLite:
    case NOrmDatabase::Interbase:
    case NOrmDatabase::DB2:
    case NOrmDatabase::DaMeng:
        return QString("datetime");
    case NOrmDatabase::PostgreSQL:
        return QLatin1String("timestamp");
    }
    return QString();
}

QString NOrmMetaModel::getDoubleType(NOrmDatabase::DatabaseType databaseType) const
{
    switch (databaseType) {
    case NOrmDatabase::UnknownDB:
    case NOrmDatabase::MSSqlServer:
    case NOrmDatabase::MySqlServer:
    case NOrmDatabase::PostgreSQL:
    case NOrmDatabase::Oracle:
    case NOrmDatabase::Sybase:
    case NOrmDatabase::SQLite:
    case NOrmDatabase::Interbase:
    case NOrmDatabase::DB2:
    case NOrmDatabase::DaMeng:
        return QString("real");
    }
    return QString();
}

QString NOrmMetaModel::getIntType(NOrmDatabase::DatabaseType databaseType) const
{
    switch (databaseType) {
    case NOrmDatabase::UnknownDB:
    case NOrmDatabase::MySqlServer:
    case NOrmDatabase::PostgreSQL:
    case NOrmDatabase::Oracle:
    case NOrmDatabase::Sybase:
    case NOrmDatabase::SQLite:
    case NOrmDatabase::Interbase:
    case NOrmDatabase::DB2:
    case NOrmDatabase::DaMeng:
        return QString("integer");
    case NOrmDatabase::MSSqlServer:
        return QLatin1String("int");
    }
    return QString();
}

QString NOrmMetaModel::getLongLongType(NOrmDatabase::DatabaseType databaseType) const
{
    switch (databaseType) {
    case NOrmDatabase::UnknownDB:
    case NOrmDatabase::MSSqlServer:
    case NOrmDatabase::MySqlServer:
    case NOrmDatabase::PostgreSQL:
    case NOrmDatabase::Oracle:
    case NOrmDatabase::Sybase:
    case NOrmDatabase::SQLite:
    case NOrmDatabase::Interbase:
    case NOrmDatabase::DB2:
    case NOrmDatabase::DaMeng:
        return QString("bigint");
    }
    return QString();
}

QString NOrmMetaModel::getStringType(NOrmDatabase::DatabaseType databaseType, int maxLength) const
{
    switch (databaseType) {
    case NOrmDatabase::UnknownDB:
    case NOrmDatabase::MySqlServer:
    case NOrmDatabase::PostgreSQL:
    case NOrmDatabase::Oracle:
    case NOrmDatabase::Sybase:
    case NOrmDatabase::SQLite:
    case NOrmDatabase::Interbase:
    case NOrmDatabase::DB2:
    case NOrmDatabase::DaMeng:
        if (maxLength > 0) {
            return QLatin1String("varchar(") + QString::number(maxLength) + QLatin1Char(')');
        } else {
            return QLatin1String("text");
        }
    case NOrmDatabase::MSSqlServer:
        if (maxLength > 0) {
            return QLatin1String("nvarchar(") + QString::number(maxLength) + QLatin1Char(')');
        } else {
            return QLatin1String("nvarchar(max)");
        }
    }
    return QString();
}

QString NOrmMetaModel::getTimeType(NOrmDatabase::DatabaseType databaseType) const
{
    switch (databaseType) {
    case NOrmDatabase::UnknownDB:
    case NOrmDatabase::MSSqlServer:
    case NOrmDatabase::MySqlServer:
    case NOrmDatabase::PostgreSQL:
    case NOrmDatabase::Oracle:
    case NOrmDatabase::Sybase:
    case NOrmDatabase::SQLite:
    case NOrmDatabase::Interbase:
    case NOrmDatabase::DB2:
    case NOrmDatabase::DaMeng:
        return QString("time");
    }
    return QString();
}

QString NOrmMetaModel::getStringListType(NOrmDatabase::DatabaseType databaseType, int maxLength) const
{
    return getStringType(databaseType,maxLength);
}

QString NOrmMetaModel::attributeNull(NOrmDatabase::DatabaseType databaseType) const
{
    switch (databaseType) {
    case NOrmDatabase::UnknownDB:
    case NOrmDatabase::MSSqlServer:
    case NOrmDatabase::MySqlServer:
    case NOrmDatabase::PostgreSQL:
    case NOrmDatabase::Oracle:
    case NOrmDatabase::Sybase:
    case NOrmDatabase::SQLite:
    case NOrmDatabase::Interbase:
    case NOrmDatabase::DB2:
    case NOrmDatabase::DaMeng:
        return QString("NOT NULL");
    }
    return QString();
}

QString NOrmMetaModel::attributeUnique(NOrmDatabase::DatabaseType databaseType) const
{
    switch (databaseType) {
    case NOrmDatabase::UnknownDB:
    case NOrmDatabase::MSSqlServer:
    case NOrmDatabase::MySqlServer:
    case NOrmDatabase::PostgreSQL:
    case NOrmDatabase::Oracle:
    case NOrmDatabase::Sybase:
    case NOrmDatabase::SQLite:
    case NOrmDatabase::Interbase:
    case NOrmDatabase::DB2:
    case NOrmDatabase::DaMeng:
        return QString("UNIQUE");
    }
    return QString();
}

QString NOrmMetaModel::attributePrimaryKey(NOrmDatabase::DatabaseType databaseType) const
{
    switch (databaseType) {
    case NOrmDatabase::UnknownDB:
    case NOrmDatabase::MSSqlServer:
    case NOrmDatabase::MySqlServer:
    case NOrmDatabase::PostgreSQL:
    case NOrmDatabase::Oracle:
    case NOrmDatabase::Sybase:
    case NOrmDatabase::SQLite:
    case NOrmDatabase::Interbase:
    case NOrmDatabase::DB2:
    case NOrmDatabase::DaMeng:
        return QString("PRIMARY KEY");
    }
    return QString();
}

QString NOrmMetaModel::attributeAutoIncrement(NOrmDatabase::DatabaseType databaseType, const NOrmMetaField &field) const
{
    QSqlDatabase db = NOrm::database();
    QSqlDriver *driver = db.driver();
    switch (databaseType) {
    case NOrmDatabase::UnknownDB:
    case NOrmDatabase::Oracle:
    case NOrmDatabase::Sybase:
    case NOrmDatabase::Interbase:
    case NOrmDatabase::DB2:
        return QString();
    case NOrmDatabase::MSSqlServer:
    case NOrmDatabase::DaMeng:
        return QLatin1String("IDENTITY(1,1)");
    case NOrmDatabase::MySqlServer:
        return QLatin1String("AUTO_INCREMENT");
    case NOrmDatabase::PostgreSQL:
        return  driver->escapeIdentifier(field.column(), QSqlDriver::FieldName) + QLatin1String(" serial PRIMARY KEY");
    case NOrmDatabase::SQLite:
        return QLatin1String("AUTOINCREMENT");
    }
    return QString();
}

bool NOrmMetaModel::remove(QObject *model) const
{
    const QVariant pk = model->property(d->primaryKey);
    NOrmQuerySetPrivate qs(model->metaObject()->className());
    qs.addFilter(NOrmWhere(QLatin1String("pk"), NOrmWhere::Equals, pk));
    return qs.sqlDelete();
}

bool NOrmMetaModel::save(QObject *model) const
{
    // find primary key
    const NOrmMetaField primaryKey = localField("pk");
    const QVariant pk = model->property(d->primaryKey);
    if (!pk.isNull() && !(primaryKey.d->type == QVariant::Int && !pk.toInt()))
    {
        QSqlDatabase db = NOrm::database();
        NOrmQuery query(db);
        query.prepare(QString::fromLatin1("SELECT 1 AS a FROM %1 WHERE %2 = ?").arg(
                          db.driver()->escapeIdentifier(d->table, QSqlDriver::FieldName),
                          db.driver()->escapeIdentifier(primaryKey.column(), QSqlDriver::FieldName)));
        query.addBindValue(pk);
        if (query.exec() && query.next())
        {
            // prepare data
            QVariantMap fields;
            foreach (const NOrmMetaField &field, d->localFields) {
                if (field.d->name != d->primaryKey) {
                    const QVariant value = model->property(field.d->name);
                    fields.insert(QString::fromLatin1(field.d->name), field.toDatabase(value));
                }
            }

            // perform UPDATE
            NOrmQuerySetPrivate qs(model->metaObject()->className());
            qs.addFilter(NOrmWhere(QLatin1String("pk"), NOrmWhere::Equals, pk));
            return qs.sqlUpdate(fields) != -1;
        }
    }

    // prepare data
    QVariantMap fields;
    foreach (const NOrmMetaField &field, d->localFields) {
        if (!field.d->autoIncrement) {
            const QVariant value = model->property(field.d->name);
            fields.insert(field.name(), field.toDatabase(value));
        }
    }

    // perform INSERT
    NOrmQuerySetPrivate qs(model->metaObject()->className());
    if (primaryKey.d->autoIncrement) {
        // fetch autoincrement pk
        QVariant insertId;
        if (!qs.sqlInsert(fields, &insertId))
            return false;
        model->setProperty(d->primaryKey, insertId);
    } else {
        if (!qs.sqlInsert(fields))
            return false;
    }
    return true;
}

