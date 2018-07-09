#include <QDebug>
#include <QSqlDriver>
#include <QSqlRecord>

#include "NOrm.h"
#include "NOrm_p.h"
#include "NOrmQuerySet.h"
#include "NOrmWhere_p.h"

NOrmCompiler::NOrmCompiler(const char *modelName, const QSqlDatabase &db)
{
    driver = db.driver();
    baseModel = NOrm::metaModel(modelName);
}

QString NOrmCompiler::referenceModel(const QString &modelPath, NOrmMetaModel *metaModel, bool nullable)
{
    if (modelPath.isEmpty())
        return driver->escapeIdentifier(baseModel.table(), QSqlDriver::TableName);

    if (modelRefs.contains(modelPath))
        return modelRefs.value(modelPath).tableReference;

    const QString modelRef = QLatin1String("T") + QString::number(modelRefs.size());
    modelRefs.insert(modelPath, NOrmModelReference(modelRef, *metaModel, nullable));
    return modelRef;
}

QString NOrmCompiler::databaseColumn(const QString &name)
{
    NOrmMetaModel model = baseModel;
    QString modelName;
    QString modelPath;
    QString modelRef = referenceModel(QString(), &model, false);
    QStringList bits = name.split(QLatin1String("__"));

    while (bits.size() > 1) {
        const QByteArray fk = bits.first().toLatin1();
        NOrmMetaModel foreignModel;
        bool foreignNullable = false;

        if (!modelPath.isEmpty())
            modelPath += QLatin1String("__");
        modelPath += bits.first();

        if (!model.foreignFields().contains(fk)) {
            // this might be a reverse relation, so look for the model
            // and if it exists continue
            foreignModel = NOrm::metaModel(fk);
            NOrmReverseReference rev;
            const QMap<QByteArray, QByteArray> foreignFields = foreignModel.foreignFields();
            foreach (const QByteArray &foreignKey, foreignFields.keys()) {
                if (foreignFields[foreignKey] == baseModel.className()) {
                    rev.leftHandKey = foreignModel.localField(foreignKey + "_id").column();
                    break;
                }
            }

            if (rev.leftHandKey.isEmpty()) {
                qWarning() << "Invalid field lookup" << name;
                return QString();
            }
            rev.rightHandKey = foreignModel.primaryKey();
            reverseModelRefs[modelPath] = rev;
        } else {
            foreignModel = NOrm::metaModel(model.foreignFields()[fk]);
            foreignNullable = model.localField(fk + QByteArray("_id")).isNullable();;
        }

        // store reference
        modelRef = referenceModel(modelPath, &foreignModel, foreignNullable);
        modelName = fk;

        model = foreignModel;
        bits.takeFirst();
    }

    const NOrmMetaField field = model.localField(bits.join(QLatin1String("__")).toLatin1());
    return modelRef + QLatin1Char('.') + driver->escapeIdentifier(field.column(), QSqlDriver::FieldName);
}

QStringList NOrmCompiler::fieldNames(bool recurse, const QStringList *fields, NOrmMetaModel *metaModel, const QString &modelPath, bool nullable)
{
    QStringList columns;
    if (!metaModel)
        metaModel = &baseModel;

    // store reference
    const QString tableName = referenceModel(modelPath, metaModel, nullable);
    foreach (const NOrmMetaField &field, metaModel->localFields())
        columns << tableName + QLatin1Char('.') + driver->escapeIdentifier(field.column(), QSqlDriver::FieldName);
    if (!recurse)
        return columns;

    // recurse for foreign keys
    const QString pathPrefix = modelPath.isEmpty() ? QString() : (modelPath + QLatin1String("__"));
    foreach (const QByteArray &fkName, metaModel->foreignFields().keys()) {
        NOrmMetaModel metaForeign = NOrm::metaModel(metaModel->foreignFields()[fkName]);
        bool nullableForeign = metaModel->localField(fkName + QByteArray("_id")).isNullable();
        QString fkS(fkName);
        if ( (fields != 0) && (fields->contains(fkS) ) )
        {
            QStringList nsl = fields->filter(QRegExp("^" + fkS + "__")).replaceInStrings(QRegExp("^" + fkS + "__"),"");
            columns += fieldNames(recurse, &nsl, &metaForeign, pathPrefix + QString::fromLatin1(fkName), nullableForeign);
        }

        if (fields == 0)
        {
            columns += fieldNames(recurse, 0, &metaForeign, pathPrefix + QString::fromLatin1(fkName), nullableForeign);
        }
    }
    return columns;
}

QString NOrmCompiler::fromSql()
{
    QString from = driver->escapeIdentifier(baseModel.table(), QSqlDriver::TableName);
    foreach (const QString &name, modelRefs.keys()) {
        const NOrmModelReference &ref = modelRefs[name];

        QString leftHandColumn, rightHandColumn;
        if (reverseModelRefs.contains(name)) {
            const NOrmReverseReference &rev = reverseModelRefs[name];
            leftHandColumn = ref.tableReference + "." + driver->escapeIdentifier(rev.leftHandKey, QSqlDriver::FieldName);;
            rightHandColumn = databaseColumn(rev.rightHandKey);
        } else {
            leftHandColumn = databaseColumn(name + QLatin1String("__pk"));
            rightHandColumn = databaseColumn(name + QLatin1String("_id"));
        }
        from += QString::fromLatin1(" %1 %2 %3 ON %4 = %5")
            .arg(ref.nullable ? "LEFT OUTER JOIN" : "INNER JOIN")
            .arg(driver->escapeIdentifier(ref.metaModel.table(), QSqlDriver::TableName))
            .arg(ref.tableReference)
            .arg(leftHandColumn)
            .arg(rightHandColumn);
    }
    return from;
}

QString NOrmCompiler::orderLimitSql(const QStringList &orderBy, int lowMark, int highMark)
{
    QString limit;

    // order
    QStringList bits;
    QString field;
    foreach (field, orderBy) {
        QString order = QLatin1String("ASC");
        if (field.startsWith(QLatin1Char('-'))) {
            order = QLatin1String("DESC");
            field = field.mid(1);
        } else if (field.startsWith(QLatin1Char('+'))) {
            field = field.mid(1);
        }
        bits.append(databaseColumn(field) + QLatin1Char(' ') + order);
    }

    if (!bits.isEmpty())
        limit += QLatin1String(" ORDER BY ") + bits.join(QLatin1String(", "));

    // limits
    NOrmDatabase::DatabaseType databaseType =
        NOrmDatabase::databaseType(NOrm::database());

    if (databaseType == NOrmDatabase::MSSqlServer) {
        if (limit.isEmpty() && (highMark > 0 || lowMark > 0))
            limit += QLatin1String(" ORDER BY ") + databaseColumn(baseModel.primaryKey());

        if (lowMark > 0 || (lowMark == 0 && highMark > 0)) {
            limit += QLatin1String(" OFFSET ") + QString::number(lowMark);
            limit += QLatin1String(" ROWS");
        }

        if (highMark > 0)
            limit += QString(" FETCH NEXT %1 ROWS ONLY").arg(highMark - lowMark);
    } else {
        if (highMark > 0)
            limit += QLatin1String(" LIMIT ") + QString::number(highMark - lowMark);

        if (lowMark > 0) {
            // no-limit is backend specific
            if (highMark <= 0) {
                if (databaseType == NOrmDatabase::SQLite)
                    limit += QLatin1String(" LIMIT -1");
                else if (databaseType == NOrmDatabase::MySqlServer)
                    // 2^64 - 1, as recommended by the MySQL documentation
                    limit += QLatin1String(" LIMIT 18446744073709551615");
            }

            limit += QLatin1String(" OFFSET ") + QString::number(lowMark);
        }
    }

    return limit;
}

void NOrmCompiler::resolve(NOrmWhere &where)
{
    // resolve column
    if (where.d->operation != NOrmWhere::None)
        where.d->key = databaseColumn(where.d->key);

    // recurse into children
    for (int i = 0; i < where.d->children.size(); i++)
        resolve(where.d->children[i]);
}

NOrmQuerySetPrivate::NOrmQuerySetPrivate(const char *modelName)
    : counter(1),
    hasResults(false),
    lowMark(0),
    highMark(0),
    selectRelated(false),
    m_modelName(modelName)
{
}

void NOrmQuerySetPrivate::addFilter(const NOrmWhere &where)
{
    // it is not possible to add filters once a limit has been set
    Q_ASSERT(!lowMark && !highMark);

    whereClause = whereClause && where;
}

NOrmWhere NOrmQuerySetPrivate::resolvedWhere(const QSqlDatabase &db) const
{
    NOrmCompiler compiler(m_modelName, db);
    NOrmWhere resolvedWhere(whereClause);
    compiler.resolve(resolvedWhere);
    return resolvedWhere;
}

bool NOrmQuerySetPrivate::sqlDelete()
{
    // DELETE on an empty queryset doesn't need a query
    if (whereClause.isNone())
        return true;

    // FIXME : it is not possible to remove entries once a limit has been set
    // because SQLite does not support limits on DELETE unless compiled with the
    // SQLITE_ENABLE_UPDATE_DELETE_LIMIT option
    if (lowMark || highMark)
        return false;

    // execute query
    NOrmQuery query(deleteQuery());
    if (!query.exec())
        return false;

    // invalidate cache
    if (hasResults) {
        properties.clear();
        hasResults = false;
    }
    return true;
}

bool NOrmQuerySetPrivate::sqlFetch()
{
    if (hasResults || whereClause.isNone())
        return true;

    // execute query
    NOrmQuery query(selectQuery());
    if (!query.exec())
        return false;

    // store results
    while (query.next()) {
        QVariantList props;
        const int propCount = query.record().count();
        for (int i = 0; i < propCount; ++i)
            props << query.value(i);
        properties.append(props);
    }
    hasResults = true;
    return true;
}

bool NOrmQuerySetPrivate::sqlInsert(const QVariantMap &fields, QVariant *insertId)
{
    // execute query
    NOrmQuery query(insertQuery(fields));
    if (!query.exec())
        return false;

    // fetch autoincrement pk
    if (insertId) {
        QSqlDatabase db = NOrm::database();
        NOrmDatabase::DatabaseType databaseType = NOrmDatabase::databaseType(db);

        if (databaseType == NOrmDatabase::PostgreSQL) {
            const NOrmMetaModel metaModel = NOrm::metaModel(m_modelName);
            NOrmQuery query(db);
            const NOrmMetaField primaryKey = metaModel.localField("pk");
            const QString seqName = db.driver()->escapeIdentifier(metaModel.table() + QLatin1Char('_') + primaryKey.column() + QLatin1String("_seq"), QSqlDriver::FieldName);
            if (!query.exec(QLatin1String("SELECT CURRVAL('") + seqName + QLatin1String("')")) || !query.next())
                return false;
            *insertId = query.value(0);
        } else {
            *insertId = query.lastInsertId();
        }
    }

    // invalidate cache
    if (hasResults) {
        properties.clear();
        hasResults = false;
    }

    return true;
}

bool NOrmQuerySetPrivate::sqlLoad(QObject *model, int index)
{
    if (!sqlFetch())
        return false;

    if (index < 0 || index >= properties.size())
    {
        qWarning("QDjangoQuerySet out of bounds");
        return false;
    }

    const NOrmMetaModel metaModel = NOrm::metaModel(m_modelName);
    int pos = 0;
    metaModel.load(model, properties.at(index), pos, this->relatedFields);
    return true;
}

static QString aggregationToString(NOrmWhere::AggregateType type){
    switch (type) {
    case NOrmWhere::AVG: return QLatin1String("AVG");
    case NOrmWhere::COUNT: return QLatin1String("COUNT");
    case NOrmWhere::SUM: return QLatin1String("SUM");
    case NOrmWhere::MIN: return QLatin1String("MIN");
    case NOrmWhere::MAX: return QLatin1String("MAX");
    }
    return QString();
}

/**
 * @brief QDjangoQuerySetPrivate::aggregateQuery Returns the SQL query to perform a @param func on the current set.
 * @param field name or expression to aggregate (eq price or amount*price)
 * @param func one of [AVG, COUNT, SUM, MIN, MAX]
 * @return SQL query to perform a @param func on the current set.
 */
NOrmQuery NOrmQuerySetPrivate::aggregateQuery(const NOrmWhere::AggregateType func, const QString &field) const
{
    QSqlDatabase db = NOrm::database();

    // build query
    NOrmCompiler compiler(m_modelName, db);
    NOrmWhere resolvedWhere(whereClause);
    compiler.resolve(resolvedWhere);

    const QString where = resolvedWhere.sql(db);
    const QString limit = compiler.orderLimitSql(QStringList(), lowMark, highMark);

    QString sql = QLatin1String("SELECT ") + aggregationToString(func)+"("+field+") "+"FROM " + compiler.fromSql();
    if (!where.isEmpty())
        sql += QLatin1String(" WHERE ") + where;
    sql += limit;
    NOrmQuery query(db);
    query.prepare(sql);
    resolvedWhere.bindValues(query);
    return query;
}

/** Returns the SQL query to perform a DELETE on the current set.
 */
NOrmQuery NOrmQuerySetPrivate::deleteQuery() const
{
    QSqlDatabase db = NOrm::database();

    // build query
    NOrmCompiler compiler(m_modelName, db);
    NOrmWhere resolvedWhere(whereClause);
    compiler.resolve(resolvedWhere);

    const QString where = resolvedWhere.sql(db);
    const QString limit = compiler.orderLimitSql(orderBy, lowMark, highMark);
    QString sql = QLatin1String("DELETE FROM ") + compiler.fromSql();
    if (!where.isEmpty())
        sql += QLatin1String(" WHERE ") + where;
    sql += limit;
    NOrmQuery query(db);
    query.prepare(sql);
    resolvedWhere.bindValues(query);

    return query;
}

/** Returns the SQL query to perform an INSERT for the specified \a fields.
 */
NOrmQuery NOrmQuerySetPrivate::insertQuery(const QVariantMap &fields) const
{
    QSqlDatabase db = NOrm::database();
    const NOrmMetaModel metaModel = NOrm::metaModel(m_modelName);

    // perform INSERT
    QStringList fieldColumns;
    QStringList fieldHolders;
    foreach (const QString &name, fields.keys()) {
        const NOrmMetaField field = metaModel.localField(name.toLatin1());
        fieldColumns << db.driver()->escapeIdentifier(field.column(), QSqlDriver::FieldName);
        fieldHolders << QLatin1String("?");
    }

    NOrmQuery query(db);
    query.prepare(QString::fromLatin1("INSERT INTO %1 (%2) VALUES(%3)").arg(
                  db.driver()->escapeIdentifier(metaModel.table(), QSqlDriver::TableName),
                  fieldColumns.join(QLatin1String(", ")), fieldHolders.join(QLatin1String(", "))));
    foreach (const QString &name, fields.keys())
        query.addBindValue(fields.value(name));
    return query;
}

/** Returns the SQL query to perform a SELECT on the current set.
 */
NOrmQuery NOrmQuerySetPrivate::selectQuery() const
{
    QSqlDatabase db = NOrm::database();

    // build query
    NOrmCompiler compiler(m_modelName, db);
    NOrmWhere resolvedWhere(whereClause);
    compiler.resolve(resolvedWhere);

    const QStringList columns = compiler.fieldNames(selectRelated, &this->relatedFields);
    const QString where = resolvedWhere.sql(db);
    const QString limit = compiler.orderLimitSql(orderBy, lowMark, highMark);
    QString sql = QLatin1String("SELECT ") + columns.join(QLatin1String(", ")) + QLatin1String(" FROM ") + compiler.fromSql();
    if (!where.isEmpty())
        sql += QLatin1String(" WHERE ") + where;
    sql += limit;
    NOrmQuery query(db);
    query.prepare(sql);
    resolvedWhere.bindValues(query);

    return query;
}

/** Returns the SQL query to perform an UPDATE on the current set for the
    specified \a fields.
 */
NOrmQuery NOrmQuerySetPrivate::updateQuery(const QVariantMap &fields) const
{
    QSqlDatabase db = NOrm::database();
    const NOrmMetaModel metaModel = NOrm::metaModel(m_modelName);

    // build query
    NOrmCompiler compiler(m_modelName, db);
    NOrmWhere resolvedWhere(whereClause);
    compiler.resolve(resolvedWhere);

    QString sql = QLatin1String("UPDATE ") + compiler.fromSql();

    // add SET
    QStringList fieldAssign;
    foreach (const QString &name, fields.keys()) {
        const NOrmMetaField field = metaModel.localField(name.toLatin1());
        fieldAssign << db.driver()->escapeIdentifier(field.column(), QSqlDriver::FieldName) + QLatin1String(" = ?");
    }
    sql += QLatin1String(" SET ") + fieldAssign.join(QLatin1String(", "));

    // add WHERE
    const QString where = resolvedWhere.sql(db);
    if (!where.isEmpty())
        sql += QLatin1String(" WHERE ") + where;

    NOrmQuery query(db);
    query.prepare(sql);
    foreach (const QString &name, fields.keys())
        query.addBindValue(fields.value(name));
    resolvedWhere.bindValues(query);

    return query;
}

int NOrmQuerySetPrivate::sqlUpdate(const QVariantMap &fields)
{
    // UPDATE on an empty queryset doesn't need a query
    if (whereClause.isNone() || fields.isEmpty())
        return 0;

    // FIXME : it is not possible to update entries once a limit has been set
    // because SQLite does not support limits on UPDATE unless compiled with the
    // SQLITE_ENABLE_UPDATE_DELETE_LIMIT option
    if (lowMark || highMark)
        return -1;

    // execute query
    NOrmQuery query(updateQuery(fields));
    if (!query.exec())
        return -1;

    // invalidate cache
    if (hasResults) {
        properties.clear();
        hasResults = false;
    }

    return query.numRowsAffected();
}

QList<QVariantMap> NOrmQuerySetPrivate::sqlValues(const QStringList &fields)
{
    QList<QVariantMap> values;
    if (!sqlFetch())
        return values;

    const NOrmMetaModel metaModel = NOrm::metaModel(m_modelName);

    // build field list
    const QList<NOrmMetaField> localFields = metaModel.localFields();
    QMap<QString, int> fieldPos;
    if (fields.isEmpty()) {
        for (int i = 0; i < localFields.size(); ++i)
            fieldPos.insert(localFields[i].name(), i);
    } else {
        foreach (const QString &name, fields) {
            int pos = 0;
            foreach (const NOrmMetaField &field, localFields) {
                if (field.name() == name)
                    break;
                pos++;
            }
            Q_ASSERT_X(pos < localFields.size(), "NOrmQuerySet<T>::values", "unknown field requested");
            fieldPos.insert(name, pos);
        }
    }

    // extract values
    foreach (const QVariantList &props, properties) {
        QVariantMap map;
        QMap<QString, int>::const_iterator i;
        for (i = fieldPos.constBegin(); i != fieldPos.constEnd(); ++i)
            map[i.key()] = props[i.value()];
        values.append(map);
    }
    return values;
}

QList<QVariantList> NOrmQuerySetPrivate::sqlValuesList(const QStringList &fields)
{
    QList<QVariantList> values;
    if (!sqlFetch())
        return values;

    const NOrmMetaModel metaModel = NOrm::metaModel(m_modelName);

    // build field list
    const QList<NOrmMetaField> localFields = metaModel.localFields();
    QList<int> fieldPos;
    if (fields.isEmpty()) {
        for (int i = 0; i < localFields.size(); ++i)
            fieldPos << i;
    } else {
        foreach (const QString &name, fields) {
            int pos = 0;
            foreach (const NOrmMetaField &field, localFields) {
                if (field.name() == name)
                    break;
                pos++;
            }
            Q_ASSERT_X(pos < localFields.size(), "QDjangoQuerySet<T>::valuesList", "unknown field requested");
            fieldPos << pos;
        }
    }

    // extract values
    foreach (const QVariantList &props, properties) {
        QVariantList list;
        foreach (int pos, fieldPos)
            list << props.at(pos);
        values.append(list);
    }
    return values;
}


/// \endcond
