#ifndef NORM_MODEL_H
#define NORM_MODEL_H

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
    NOrmModel(QObject *parent = 0);

    QVariant pk() const;
    void setPk(const QVariant &pk);

public slots:
    bool remove();
    bool save();
    QString toString() const;

protected:
    QObject *foreignKey(const char *name) const;
    void setForeignKey(const char *name, QObject *value);
};

#endif
