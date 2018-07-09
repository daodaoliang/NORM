#include <QDebug>
#include <QStringList>

#include "NOrm.h"
#include "NOrmModel.h"
#include "NOrmQuerySet.h"

NOrmModel::NOrmModel(QObject *parent)
    : QObject(parent)
{
}

QVariant NOrmModel::pk() const
{
    const NOrmMetaModel metaModel = NOrm::metaModel(metaObject()->className());
    return property(metaModel.primaryKey());
}

void NOrmModel::setPk(const QVariant &pk)
{
    const NOrmMetaModel metaModel = NOrm::metaModel(metaObject()->className());
    setProperty(metaModel.primaryKey(), pk);
}

QObject *NOrmModel::foreignKey(const char *name) const
{
    const NOrmMetaModel metaModel = NOrm::metaModel(metaObject()->className());
    return metaModel.foreignKey(this, name);
}

void NOrmModel::setForeignKey(const char *name, QObject *value)
{
    const NOrmMetaModel metaModel = NOrm::metaModel(metaObject()->className());
    metaModel.setForeignKey(this, name, value);
}

bool NOrmModel::remove()
{
    const NOrmMetaModel metaModel = NOrm::metaModel(metaObject()->className());
    return metaModel.remove(this);
}

bool NOrmModel::save()
{
    const NOrmMetaModel metaModel = NOrm::metaModel(metaObject()->className());
    return metaModel.save(this);
}

/** Returns a string representation of the model instance.
 */
QString NOrmModel::toString() const
{
    const NOrmMetaModel metaModel = NOrm::metaModel(metaObject()->className());
    const QByteArray pkName = metaModel.primaryKey();
    return QString::fromLatin1("%1(%2=%3)").arg(QString::fromLatin1(metaObject()->className()), QString::fromLatin1(pkName), property(pkName).toString());
}

