#ifndef NORMMETAMODEL_H
#define NORMMETAMODEL_H

#include <QMap>
#include <QSharedDataPointer>
#include <QVariant>
#include <QStringList>
#include "NOrm_p.h"

class NOrmMetaFieldPrivate;
class NOrmMetaModelPrivate;

class NOrmMetaField
{
public:
    NOrmMetaField();
    NOrmMetaField(const NOrmMetaField &other);
    ~NOrmMetaField();
    NOrmMetaField& operator=(const NOrmMetaField &other);

    QString column() const;
    bool isAutoIncrement() const;
    bool isBlank() const;
    bool isNullable() const;
    bool isUnique() const;
    bool isValid() const;
    QString name() const;
    int maxLength() const;
    QVariant toDatabase(const QVariant &value) const;
private:
    QSharedDataPointer<NOrmMetaFieldPrivate> d;
    friend class NOrmMetaModel;
};

class NOrmMetaModel
{
public:
    NOrmMetaModel(const QMetaObject *model = 0);
    NOrmMetaModel(const NOrmMetaModel &other);
    ~NOrmMetaModel();
    NOrmMetaModel& operator=(const NOrmMetaModel &other);

    bool isValid() const;

    bool createTable() const;
    QStringList createTableSql() const;
    bool dropTable() const;

    void load(QObject *model, const QVariantList &props, int &pos, const QStringList &relatedFields = QStringList()) const;
    bool remove(QObject *model) const;
    bool save(QObject *model) const;

    QObject *foreignKey(const QObject *model, const char *name) const;
    void setForeignKey(QObject *model, const char *name, QObject *value) const;

    QString className() const;
    NOrmMetaField localField(const char *name) const;
    QList<NOrmMetaField> localFields() const;
    QMap<QByteArray, QByteArray> foreignFields() const;
    QByteArray primaryKey() const;
    QString table() const;

private:
    QString getBoolType(NOrmDatabase::DatabaseType databaseType) const;
    QString getByteArrayType(NOrmDatabase::DatabaseType databaseType, int maxLength) const;
    QString getDateType(NOrmDatabase::DatabaseType databaseType) const;
    QString getDateTimeType(NOrmDatabase::DatabaseType databaseType) const;
    QString getDoubleType(NOrmDatabase::DatabaseType databaseType) const;
    QString getIntType(NOrmDatabase::DatabaseType databaseType) const;
    QString getLongLongType(NOrmDatabase::DatabaseType databaseType) const;
    QString getStringType(NOrmDatabase::DatabaseType databaseType, int maxLength) const;
    QString getTimeType(NOrmDatabase::DatabaseType databaseType) const;
    QString getStringListType(NOrmDatabase::DatabaseType databaseType, int maxLength) const;

    QString attributeNull(NOrmDatabase::DatabaseType databaseType) const;
    QString attributeUnique(NOrmDatabase::DatabaseType databaseType) const;
    QString attributePrimaryKey(NOrmDatabase::DatabaseType databaseType) const;
    QString attributeAutoIncrement(NOrmDatabase::DatabaseType databaseType, const NOrmMetaField &field) const;


private:
    QSharedDataPointer<NOrmMetaModelPrivate> d;
};

#endif
