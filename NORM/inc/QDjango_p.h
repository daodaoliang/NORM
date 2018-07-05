/*
 * Copyright (C) 2010-2015 Jeremy Lainé
 * Contact: https://github.com/jlaine/qdjango
 *
 * This file is part of the QDjango Library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#ifndef QDJANGO_P_H
#define QDJANGO_P_H

#include <QMap>
#include <QMutex>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#if defined(NORM_LIBRARY)
#  define NORMSHARED_EXPORT Q_DECL_EXPORT
#else
#  define NORMSHARED_EXPORT Q_DECL_IMPORT
#endif

/** \brief The QDjangoDatabase class represents a set of connections to a
 *  database.
 *
 * \internal
 */
class NORMSHARED_EXPORT QDjangoDatabase : public QObject
{
    Q_OBJECT

public:
    QDjangoDatabase(QObject *parent = 0);

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

class NORMSHARED_EXPORT QDjangoQuery : public QSqlQuery
{
public:
    QDjangoQuery(QSqlDatabase db);
    void addBindValue(const QVariant &val, QSql::ParamType paramType = QSql::In);
    bool exec();
    bool exec(const QString &query);
};

#endif
