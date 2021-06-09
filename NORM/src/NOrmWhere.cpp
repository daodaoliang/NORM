#include <QStringList>
#include <QDebug>

#include "NOrm.h"
#include "NOrmWhere.h"
#include "NOrmWhere_p.h"

static QString escapeLike(const QString &data)
{
    QString escaped = data;
    escaped.replace(QLatin1String("%"), QLatin1String("\\%"));
    escaped.replace(QLatin1String("_"), QLatin1String("\\_"));
    return escaped;
}

/// \cond

NOrmWherePrivate::NOrmWherePrivate()
    : operation(NOrmWhere::None)
    , combine(NoCombine)
    , negate(false)
{
}

NOrmWhere::NOrmWhere()
{
    d = new NOrmWherePrivate;
}

NOrmWhere::NOrmWhere(const NOrmWhere &other)
    : d(other.d)
{
}

NOrmWhere::NOrmWhere(const QString &key, NOrmWhere::Operation operation, QVariant value)
{
    d = new NOrmWherePrivate;
    d->key = key;
    d->operation = operation;
    d->data = value;
}

NOrmWhere::~NOrmWhere()
{
}

NOrmWhere& NOrmWhere::operator=(const NOrmWhere& other)
{
    d = other.d;
    return *this;
}

NOrmWhere NOrmWhere::operator!() const
{
    NOrmWhere result;
    result.d = d;
    if (d->children.isEmpty()) {
        switch (d->operation)
        {
        case None:
        case IsIn:
        case StartsWith:
        case IStartsWith:
        case EndsWith:
        case IEndsWith:
        case Contains:
        case IContains:
            result.d->negate = !d->negate;
            break;
        case IsNull:
            // simplify !(is null) to is not null
            result.d->data = !d->data.toBool();
            break;
        case Equals:
            // simplify !(a = b) to a != b
            result.d->operation = NotEquals;
            break;
        case IEquals:
            // simplify !(a = b) to a != b
            result.d->operation = INotEquals;
            break;
        case NotEquals:
            // simplify !(a != b) to a = b
            result.d->operation = Equals;
            break;
        case INotEquals:
            // simplify !(a != b) to a = b
            result.d->operation = IEquals;
            break;
        case GreaterThan:
            // simplify !(a > b) to a <= b
            result.d->operation = LessOrEquals;
            break;
        case LessThan:
            // simplify !(a < b) to a >= b
            result.d->operation = GreaterOrEquals;
            break;
        case GreaterOrEquals:
            // simplify !(a >= b) to a < b
            result.d->operation = LessThan;
            break;
        case LessOrEquals:
            // simplify !(a <= b) to a > b
            result.d->operation = GreaterThan;
            break;
        }
    } else {
        result.d->negate = !d->negate;
    }

    return result;
}

NOrmWhere NOrmWhere::operator&&(const NOrmWhere &other) const
{
    if (isAll() || other.isNone())
        return other;
    else if (isNone() || other.isAll())
        return *this;

    if (d->combine == NOrmWherePrivate::AndCombine) {
        NOrmWhere result = *this;
        result.d->children << other;
        return result;
    }

    NOrmWhere result;
    result.d->combine = NOrmWherePrivate::AndCombine;
    result.d->children << *this << other;
    return result;
}

NOrmWhere NOrmWhere::operator||(const NOrmWhere &other) const
{
    if (isAll() || other.isNone())
        return *this;
    else if (isNone() || other.isAll())
        return other;

    if (d->combine == NOrmWherePrivate::OrCombine) {
        NOrmWhere result = *this;
        result.d->children << other;
        return result;
    }

    NOrmWhere result;
    result.d->combine = NOrmWherePrivate::OrCombine;
    result.d->children << *this << other;
    return result;
}

void NOrmWhere::bindValues(NOrmQuery &query) const
{
    if (d->operation == NOrmWhere::IsIn) {
        const QList<QVariant> values = d->data.toList();
        for (int i = 0; i < values.size(); i++)
            query.addBindValue(values[i]);
    } else if (d->operation == NOrmWhere::IsNull) {
        // no data to bind
    } else if (d->operation == NOrmWhere::StartsWith || d->operation == NOrmWhere::IStartsWith) {
        query.addBindValue(escapeLike(d->data.toString()) + QLatin1String("%"));
    } else if (d->operation == NOrmWhere::EndsWith || d->operation == NOrmWhere::IEndsWith) {
        query.addBindValue(QLatin1String("%") + escapeLike(d->data.toString()));
    } else if (d->operation == NOrmWhere::Contains || d->operation == NOrmWhere::IContains) {
        query.addBindValue(QLatin1String("%") + escapeLike(d->data.toString()) + QLatin1String("%"));
    } else if (d->operation != NOrmWhere::None) {
        query.addBindValue(d->data);
    } else {
        foreach (const NOrmWhere &child, d->children)
            child.bindValues(query);
    }
}

bool NOrmWhere::isAll() const
{
    return d->combine == NOrmWherePrivate::NoCombine && d->operation == None && d->negate == false;
}

bool NOrmWhere::isNone() const
{
    return d->combine == NOrmWherePrivate::NoCombine && d->operation == None && d->negate == true;
}

QString NOrmWhere::sql(const QSqlDatabase &db) const
{
    NOrmDatabase::DatabaseType databaseType = NOrmDatabase::databaseType(db);

    switch (databaseType) {
    case NOrmDatabase::UnknownDB:
    case NOrmDatabase::Oracle:
    case NOrmDatabase::Sybase:
    case NOrmDatabase::Interbase:
    case NOrmDatabase::DB2:
    case NOrmDatabase::MSSqlServer:
        return sqlDefult(db);
    case NOrmDatabase::MySqlServer:
        return sqlMysql(db);
    case NOrmDatabase::PostgreSQL:
        return sqlPostgre(db);
    case NOrmDatabase::SQLite:
        return sqlSqlite(db);
    case NOrmDatabase::DaMeng:
        return sqlDaMeng(db);
    }
    return QString();
}

QString NOrmWhere::toString() const
{
    if (d->combine == NOrmWherePrivate::NoCombine) {
        return QLatin1String("NOrmWhere(")
                  + "key=\"" + d->key + "\""
                  + ", operation=\"" + NOrmWherePrivate::operationToString(d->operation) + "\""
                  + ", value=\"" + d->data.toString() + "\""
                  + ", negate=" + (d->negate ? "true":"false")
                  + ")";
    } else {
        QStringList bits;
        foreach (const NOrmWhere &childWhere, d->children) {
            bits.append(childWhere.toString());
        }
        if (d->combine == NOrmWherePrivate::OrCombine) {
            return bits.join(" || ");
        } else {
            return bits.join(" && ");
        }
    }
}

QString NOrmWhere::sqlDefult(const QSqlDatabase &db) const
{
    switch (d->operation) {
        case Equals:
            return d->key + QLatin1String(" = ?");
        case NotEquals:
            return d->key + QLatin1String(" != ?");
        case GreaterThan:
            return d->key + QLatin1String(" > ?");
        case LessThan:
            return d->key + QLatin1String(" < ?");
        case GreaterOrEquals:
            return d->key + QLatin1String(" >= ?");
        case LessOrEquals:
            return d->key + QLatin1String(" <= ?");
        case IsIn:
        {
            QStringList bits;
            for (int i = 0; i < d->data.toList().size(); i++)
                bits << QLatin1String("?");
            if (d->negate)
                return d->key + QString::fromLatin1(" NOT IN (%1)").arg(bits.join(QLatin1String(", ")));
            else
                return d->key + QString::fromLatin1(" IN (%1)").arg(bits.join(QLatin1String(", ")));
        }
        case IsNull:
            return d->key + QLatin1String(d->data.toBool() ? " IS NULL" : " IS NOT NULL");
        case StartsWith:
        case EndsWith:
        case Contains:
        {
            QString op;
            op = QLatin1String(d->negate ? "NOT LIKE" : "LIKE");
            return d->key + QLatin1String(" ") + op + QLatin1String(" ?");
        }
        case IStartsWith:
        case IEndsWith:
        case IContains:
        case IEquals:
        {
            const QString op = QLatin1String(d->negate ? "NOT LIKE" : "LIKE");
            return d->key + QLatin1String(" ") + op + QLatin1String(" ?");
        }
        case INotEquals:
        {
            const QString op = QLatin1String(d->negate ? "LIKE" : "NOT LIKE");
            return d->key + QLatin1String(" ") + op + QLatin1String(" ?");
        }
        case None:
            if (d->combine == NOrmWherePrivate::NoCombine) {
                return d->negate ? QLatin1String("1 != 0") : QString();
            } else {
                QStringList bits;
                foreach (const NOrmWhere &child, d->children) {
                    QString atom = child.sql(db);
                    if (child.d->children.isEmpty())
                        bits << atom;
                    else
                        bits << QString::fromLatin1("(%1)").arg(atom);
                }

                QString combined;
                if (d->combine == NOrmWherePrivate::AndCombine)
                    combined = bits.join(QLatin1String(" AND "));
                else if (d->combine == NOrmWherePrivate::OrCombine)
                    combined = bits.join(QLatin1String(" OR "));
                if (d->negate)
                    combined = QString::fromLatin1("NOT (%1)").arg(combined);
                return combined;
            }
    }

    return QString();
}

QString NOrmWhere::sqlMysql(const QSqlDatabase &db) const
{
    switch (d->operation) {
        case Equals:
            return d->key + QLatin1String(" = ?");
        case NotEquals:
            return d->key + QLatin1String(" != ?");
        case GreaterThan:
            return d->key + QLatin1String(" > ?");
        case LessThan:
            return d->key + QLatin1String(" < ?");
        case GreaterOrEquals:
            return d->key + QLatin1String(" >= ?");
        case LessOrEquals:
            return d->key + QLatin1String(" <= ?");
        case IsIn:
        {
            QStringList bits;
            for (int i = 0; i < d->data.toList().size(); i++)
                bits << QLatin1String("?");
            if (d->negate)
                return d->key + QString::fromLatin1(" NOT IN (%1)").arg(bits.join(QLatin1String(", ")));
            else
                return d->key + QString::fromLatin1(" IN (%1)").arg(bits.join(QLatin1String(", ")));
        }
        case IsNull:
            return d->key + QLatin1String(d->data.toBool() ? " IS NULL" : " IS NOT NULL");
        case StartsWith:
        case EndsWith:
        case Contains:
        {
            QString op;
            op = QLatin1String(d->negate ? "NOT LIKE BINARY" : "LIKE BINARY");
            return d->key + QLatin1String(" ") + op + QLatin1String(" ?");
        }
        case IStartsWith:
        case IEndsWith:
        case IContains:
        case IEquals:
        {
            const QString op = QLatin1String(d->negate ? "NOT LIKE" : "LIKE");
            return d->key + QLatin1String(" ") + op + QLatin1String(" ?");
        }
        case INotEquals:
        {
            const QString op = QLatin1String(d->negate ? "LIKE" : "NOT LIKE");
            return d->key + QLatin1String(" ") + op + QLatin1String(" ?");
        }
        case None:
            if (d->combine == NOrmWherePrivate::NoCombine) {
                return d->negate ? QLatin1String("1 != 0") : QString();
            } else {
                QStringList bits;
                foreach (const NOrmWhere &child, d->children) {
                    QString atom = child.sql(db);
                    if (child.d->children.isEmpty())
                        bits << atom;
                    else
                        bits << QString::fromLatin1("(%1)").arg(atom);
                }

                QString combined;
                if (d->combine == NOrmWherePrivate::AndCombine)
                    combined = bits.join(QLatin1String(" AND "));
                else if (d->combine == NOrmWherePrivate::OrCombine)
                    combined = bits.join(QLatin1String(" OR "));
                if (d->negate)
                    combined = QString::fromLatin1("NOT (%1)").arg(combined);
                return combined;
            }
    }

    return QString();
}

QString NOrmWhere::sqlSqlite(const QSqlDatabase &db) const
{
    switch (d->operation) {
        case Equals:
            return d->key + QLatin1String(" = ?");
        case NotEquals:
            return d->key + QLatin1String(" != ?");
        case GreaterThan:
            return d->key + QLatin1String(" > ?");
        case LessThan:
            return d->key + QLatin1String(" < ?");
        case GreaterOrEquals:
            return d->key + QLatin1String(" >= ?");
        case LessOrEquals:
            return d->key + QLatin1String(" <= ?");
        case IsIn:
        {
            QStringList bits;
            for (int i = 0; i < d->data.toList().size(); i++)
                bits << QLatin1String("?");
            if (d->negate)
                return d->key + QString::fromLatin1(" NOT IN (%1)").arg(bits.join(QLatin1String(", ")));
            else
                return d->key + QString::fromLatin1(" IN (%1)").arg(bits.join(QLatin1String(", ")));
        }
        case IsNull:
            return d->key + QLatin1String(d->data.toBool() ? " IS NULL" : " IS NOT NULL");
        case StartsWith:
        case EndsWith:
        case Contains:
        {
            QString op;
            op = QLatin1String(d->negate ? "NOT LIKE" : "LIKE");
            return d->key + QLatin1String(" ") + op + QLatin1String(" ? ESCAPE '\\'");
        }
        case IStartsWith:
        case IEndsWith:
        case IContains:
        case IEquals:
        {
            const QString op = QLatin1String(d->negate ? "NOT LIKE" : "LIKE");
            return d->key + QLatin1String(" ") + op + QLatin1String(" ? ESCAPE '\\'");
        }
        case INotEquals:
        {
            const QString op = QLatin1String(d->negate ? "LIKE" : "NOT LIKE");
            return d->key + QLatin1String(" ") + op + QLatin1String(" ? ESCAPE '\\'");
        }
        case None:
            if (d->combine == NOrmWherePrivate::NoCombine) {
                return d->negate ? QLatin1String("1 != 0") : QString();
            } else {
                QStringList bits;
                foreach (const NOrmWhere &child, d->children) {
                    QString atom = child.sql(db);
                    if (child.d->children.isEmpty())
                        bits << atom;
                    else
                        bits << QString::fromLatin1("(%1)").arg(atom);
                }

                QString combined;
                if (d->combine == NOrmWherePrivate::AndCombine)
                    combined = bits.join(QLatin1String(" AND "));
                else if (d->combine == NOrmWherePrivate::OrCombine)
                    combined = bits.join(QLatin1String(" OR "));
                if (d->negate)
                    combined = QString::fromLatin1("NOT (%1)").arg(combined);
                return combined;
            }
    }

    return QString();
}

QString NOrmWhere::sqlPostgre(const QSqlDatabase &db) const
{
    switch (d->operation) {
        case Equals:
            return d->key + QLatin1String(" = ?");
        case NotEquals:
            return d->key + QLatin1String(" != ?");
        case GreaterThan:
            return d->key + QLatin1String(" > ?");
        case LessThan:
            return d->key + QLatin1String(" < ?");
        case GreaterOrEquals:
            return d->key + QLatin1String(" >= ?");
        case LessOrEquals:
            return d->key + QLatin1String(" <= ?");
        case IsIn:
        {
            QStringList bits;
            for (int i = 0; i < d->data.toList().size(); i++)
                bits << QLatin1String("?");
            if (d->negate)
                return d->key + QString::fromLatin1(" NOT IN (%1)").arg(bits.join(QLatin1String(", ")));
            else
                return d->key + QString::fromLatin1(" IN (%1)").arg(bits.join(QLatin1String(", ")));
        }
        case IsNull:
            return d->key + QLatin1String(d->data.toBool() ? " IS NULL" : " IS NOT NULL");
        case StartsWith:
        case EndsWith:
        case Contains:
        {
            QString op;
            op = QLatin1String(d->negate ? "NOT LIKE" : "LIKE");
            return d->key + QLatin1String(" ") + op + QLatin1String(" ?");
        }
        case IStartsWith:
        case IEndsWith:
        case IContains:
        case IEquals:
        {
            const QString op = QLatin1String(d->negate ? "NOT LIKE" : "LIKE");
            return QLatin1String("UPPER(") + d->key + QLatin1String("::text) ") + op + QLatin1String(" UPPER(?)");
        }
        case INotEquals:
        {
            const QString op = QLatin1String(d->negate ? "LIKE" : "NOT LIKE");
            return QLatin1String("UPPER(") + d->key + QLatin1String("::text) ") + op + QLatin1String(" UPPER(?)");
        }
        case None:
            if (d->combine == NOrmWherePrivate::NoCombine) {
                return d->negate ? QLatin1String("1 != 0") : QString();
            } else {
                QStringList bits;
                foreach (const NOrmWhere &child, d->children) {
                    QString atom = child.sql(db);
                    if (child.d->children.isEmpty())
                        bits << atom;
                    else
                        bits << QString::fromLatin1("(%1)").arg(atom);
                }

                QString combined;
                if (d->combine == NOrmWherePrivate::AndCombine)
                    combined = bits.join(QLatin1String(" AND "));
                else if (d->combine == NOrmWherePrivate::OrCombine)
                    combined = bits.join(QLatin1String(" OR "));
                if (d->negate)
                    combined = QString::fromLatin1("NOT (%1)").arg(combined);
                return combined;
            }
    }

    return QString();
}

QString NOrmWhere::sqlDaMeng(const QSqlDatabase &db) const
{
    QString tmp;
    if (d.data()->data.type() == QVariant::String) {
        tmp = "?";
    } else {
        tmp = "?";
    }

    switch (d->operation) {
        case Equals:
            return d->key + QString(" = %1").arg(tmp);
        case NotEquals:
            return d->key + QString(" != %1").arg(tmp);
        case GreaterThan:
            return d->key + QString(" > %1").arg(tmp);
        case LessThan:
            return d->key + QString(" < %1").arg(tmp);
        case GreaterOrEquals:
            return d->key + QString(" >= %1").arg(tmp);
        case LessOrEquals:
            return d->key + QString(" <= %1").arg(tmp);
        case IsIn:
        {
            QStringList bits;
            for (int i = 0; i < d->data.toList().size(); i++)
                bits << QString("%1").arg(tmp);
            if (d->negate)
                return d->key + QString::fromLatin1(" NOT IN (%1)").arg(bits.join(QLatin1String(", ")));
            else
                return d->key + QString::fromLatin1(" IN (%1)").arg(bits.join(QLatin1String(", ")));
        }
        case IsNull:
            return d->key + QLatin1String(d->data.toBool() ? " IS NULL" : " IS NOT NULL");
        case StartsWith:
        case EndsWith:
        case Contains:
        {
            QString op;
            op = QLatin1String(d->negate ? "NOT LIKE" : "LIKE");
            return d->key + QLatin1String(" ") + op + QString(" %1").arg(tmp);
        }
        case IStartsWith:
        case IEndsWith:
        case IContains:
        case IEquals:
        {
            const QString op = QLatin1String(d->negate ? "NOT LIKE" : "LIKE");
            return d->key + QLatin1String(" ") + op + QString(" %1").arg(tmp);
        }
        case INotEquals:
        {
            const QString op = QLatin1String(d->negate ? "LIKE" : "NOT LIKE");
            return d->key + QLatin1String(" ") + op + QString(" %1").arg(tmp);
        }
        case None:
            if (d->combine == NOrmWherePrivate::NoCombine) {
                return d->negate ? QLatin1String("1 != 0") : QString();
            } else {
                QStringList bits;
                foreach (const NOrmWhere &child, d->children) {
                    QString atom = child.sql(db);
                    if (child.d->children.isEmpty())
                        bits << atom;
                    else
                        bits << QString::fromLatin1("(%1)").arg(atom);
                }

                QString combined;
                if (d->combine == NOrmWherePrivate::AndCombine)
                    combined = bits.join(QLatin1String(" AND "));
                else if (d->combine == NOrmWherePrivate::OrCombine)
                    combined = bits.join(QLatin1String(" OR "));
                if (d->negate)
                    combined = QString::fromLatin1("NOT (%1)").arg(combined);
                return combined;
            }
    }

    return QString();
}
QString NOrmWherePrivate::operationToString(NOrmWhere::Operation operation)
{
    switch (operation) {
    case NOrmWhere::Equals: return QLatin1String("Equals");
    case NOrmWhere::IEquals: return QLatin1String("IEquals");
    case NOrmWhere::NotEquals: return QLatin1String("NotEquals");
    case NOrmWhere::INotEquals: return QLatin1String("INotEquals");
    case NOrmWhere::GreaterThan: return QLatin1String("GreaterThan");
    case NOrmWhere::LessThan: return QLatin1String("LessThan");
    case NOrmWhere::GreaterOrEquals: return QLatin1String("GreaterOrEquals");
    case NOrmWhere::LessOrEquals: return QLatin1String("LessOrEquals");
    case NOrmWhere::StartsWith: return QLatin1String("StartsWith");
    case NOrmWhere::IStartsWith: return QLatin1String("IStartsWith");
    case NOrmWhere::EndsWith: return QLatin1String("EndsWith");
    case NOrmWhere::IEndsWith: return QLatin1String("IEndsWith");
    case NOrmWhere::Contains: return QLatin1String("Contains");
    case NOrmWhere::IContains: return QLatin1String("IContains");
    case NOrmWhere::IsIn: return QLatin1String("IsIn");
    case NOrmWhere::IsNull: return QLatin1String("IsNull");
    case NOrmWhere::None:
    default:
        return QLatin1String("");
    }

    return QString();
}
