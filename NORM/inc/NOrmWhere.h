#ifndef NORM_WHERE_H
#define NORM_WHERE_H

#include <QSharedDataPointer>
#include <QVariant>

#include "NOrm_p.h"

class NOrmMetaModel;
class NOrmQuery;
class NOrmWherePrivate;

class NOrmWhere
{
public:
    enum Operation
    {
        None,
        Equals,
        NotEquals,
        GreaterThan,
        LessThan,
        GreaterOrEquals,
        LessOrEquals,
        StartsWith,
        EndsWith,
        Contains,
        IsIn,
        IsNull,
        IEquals,
        INotEquals,
        IStartsWith,
        IEndsWith,
        IContains
    };

    enum AggregateType{
        AVG,
        COUNT,
        SUM,
        MIN,
        MAX
    };
    NOrmWhere();
    NOrmWhere(const NOrmWhere &other);
    NOrmWhere(const QString &key, NOrmWhere::Operation operation, QVariant value);
    ~NOrmWhere();

    NOrmWhere& operator=(const NOrmWhere &other);
    NOrmWhere operator!() const;
    NOrmWhere operator&&(const NOrmWhere &other) const;
    NOrmWhere operator||(const NOrmWhere &other) const;

    void bindValues(NOrmQuery &query) const;
    bool isAll() const;
    bool isNone() const;
    QString sql(const QSqlDatabase &db) const;
    QString toString() const;

private:
    QString sqlDefult(const QSqlDatabase &db) const;
    QString sqlMysql(const QSqlDatabase &db) const;
    QString sqlSqlite(const QSqlDatabase &db) const;
    QString sqlPostgre(const QSqlDatabase &db) const;
    QString sqlDaMeng(const QSqlDatabase &db) const;

private:
    QSharedDataPointer<NOrmWherePrivate> d;
    friend class NOrmCompiler;
};

#endif
