#ifndef NORM_QUERYSET_P_H
#define NORM_QUERYSET_P_H

#include <QStringList>
#include "NOrm_p.h"
#include "NOrmWhere.h"

class NOrmMetaModel;

class NOrmModelReference
{
public:
    NOrmModelReference(const QString &tableReference_ = QString(), const NOrmMetaModel &metaModel_ = NOrmMetaModel(), bool nullable_ = false)
        : tableReference(tableReference_)
        , metaModel(metaModel_)
        , nullable(nullable_)
    {
    };

    QString tableReference;
    NOrmMetaModel metaModel;
    bool nullable;
};

class NOrmReverseReference
{
public:
    QString leftHandKey;
    QString rightHandKey;
};

/** \internal
 */
class NOrmCompiler
{
public:
    NOrmCompiler(const char *modelName, const QSqlDatabase &db);
    QString fromSql();
    QStringList fieldNames(bool recurse, const QStringList *fields = nullptr, NOrmMetaModel *metaModel = nullptr, const QString &modelPath = QString(), bool nullable = false);
    QString orderLimitSql(const QStringList &orderBy, int lowMark, int highMark);
    void resolve(NOrmWhere &where);

private:
    QString databaseColumn(const QString &name);
    QString referenceModel(const QString &modelPath, NOrmMetaModel *metaModel, bool nullable);
    void limitSql(QString &limit, int lowMark, int highMark);

    QSqlDriver *driver;
    NOrmMetaModel baseModel;
    QMap<QString, NOrmModelReference> modelRefs;
    QMap<QString, NOrmReverseReference> reverseModelRefs;
    QMap<QString, QString> fieldColumnCache;
};

/** \internal
 */
class NOrmQuerySetPrivate
{
public:
    NOrmQuerySetPrivate(const char *modelName);

    void addFilter(const NOrmWhere &where);
    NOrmWhere resolvedWhere(const QSqlDatabase &db) const;
    bool sqlDelete();
    bool sqlFetch();
    bool sqlInsert(const QVariantMap &fields, QVariant *insertId = nullptr);
    bool sqlLoad(QObject *model, int index);
    int sqlUpdate(const QVariantMap &fields);
    QList<QVariantMap> sqlValues(const QStringList &fields);
    QList<QVariantList> sqlValuesList(const QStringList &fields);

    // SQL queries
    NOrmQuery aggregateQuery(const NOrmWhere::AggregateType func, const QString &field) const;
    NOrmQuery deleteQuery() const;
    NOrmQuery insertQuery(const QVariantMap &fields) const;
    NOrmQuery selectQuery() const;
    NOrmQuery updateQuery(const QVariantMap &fields) const;

    // reference counter
    QAtomicInt counter;

    bool hasResults;
    int lowMark;
    int highMark;
    NOrmWhere whereClause;
    QStringList orderBy;
    QList<QVariantList> properties;
    bool selectRelated;
    QStringList relatedFields;

private:
    Q_DISABLE_COPY(NOrmQuerySetPrivate)
    QByteArray m_modelName;
    friend class NOrmMetaModel;
};

#endif
