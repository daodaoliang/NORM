#ifndef NORMMETAMODEL_H
#define NORMMETAMODEL_H

/*
 * 描述: NORM 数据表元模型
 * 作者: daodaoliang@yeah.net
 * 时间: 2021-07-16
 */

#include <QMap>
#include <QSharedDataPointer>
#include <QVariant>
#include <QStringList>
#include "NOrm_p.h"

class NOrmMetaFieldPrivate;
class NOrmMetaModelPrivate;

/**
 * @brief The NOrmMetaField class 数据表字段抽象
 */
class NOrmMetaField
{
public:

    // 构造
    NOrmMetaField();
    NOrmMetaField(const NOrmMetaField &other);

    // 析构
    ~NOrmMetaField();

    // 重载复制
    NOrmMetaField& operator=(const NOrmMetaField &other);

    // 列名
    QString column() const;

    // 是否自增
    bool isAutoIncrement() const;

    // 是否空
    bool isBlank() const;

    // 是否允许空值
    bool isNullable() const;

    // 是否唯一
    bool isUnique() const;

    // 是否有效
    bool isValid() const;

    // 列名
    QString name() const;

    // 最大长度
    int maxLength() const;

    // 转换成数据库中将要存储的数据值
    QVariant toDatabase(const QVariant &value) const;

private:
    // 私有功能对象
    QSharedDataPointer<NOrmMetaFieldPrivate> d;

    // 属于哪个元对象表
    friend class NOrmMetaModel;
};

/**
 * @brief The NOrmMetaModel class 表元对象模型
 */
class NOrmMetaModel
{
public:
    // 构造
    NOrmMetaModel(const QMetaObject *model = nullptr);
    NOrmMetaModel(const NOrmMetaModel &other);

    // 析构
    ~NOrmMetaModel();

    // 重载赋值
    NOrmMetaModel& operator=(const NOrmMetaModel &other);

    // 是否有效
    bool isValid() const;

    // 创建数据表
    bool createTable() const;

    // 建表语句
    QStringList createTableSql() const;

    // 删除数据表
    bool dropTable() const;

    void load(QObject *model, const QVariantList &props, int &pos, const QStringList &relatedFields = QStringList()) const;

    // 删除数据记录
    bool remove(QObject *model) const;

    // 插入数据记录
    bool save(QObject *model) const;

    // 外键
    QObject *foreignKey(const QObject *model, const char *name) const;
    void setForeignKey(QObject *model, const char *name, QObject *value) const;

    // 类名字
    QString className() const;

    // 本表字段信息
    NOrmMetaField localField(const char *name) const;

    // 本表字段信息
    QList<NOrmMetaField> localFields() const;

    // 外键信息
    QMap<QByteArray, QByteArray> foreignFields() const;

    // 主键信息
    QByteArray primaryKey() const;

    // 表名字
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
