#ifndef NORM_MODEL_H
#define NORM_MODEL_H

/*
 * 描述: NORM 数据表模型
 * 作者: daodaoliang@yeah.net
 * 时间: 2021-07-16
 */

#include <QObject>
#include <QVariant>
#include "NOrm_p.h"
#include "saveinthread.h"

class NOrmModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant pk READ pk WRITE setPk)
    Q_CLASSINFO("pk", "ignore_field=true")

public:
    NOrmModel(QObject *parent = nullptr);

    // 主键
    QVariant pk() const;
    void setPk(const QVariant &pk);

public slots:

    // 删除
    bool remove();

    // 保存
    bool save();

    // 转打印字串
    QString toString() const;

protected:
    QObject *foreignKey(const char *name) const;
    void setForeignKey(const char *name, QObject *value);
};

#endif
