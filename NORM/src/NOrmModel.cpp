#include <QDebug>
#include <QStringList>

#include "NOrm.h"
#include "NOrmModel.h"
#include "NOrmQuerySet.h"

/** Construct a new QDjangoModel.
 *
 * \param parent
 */
NOrmModel::NOrmModel(QObject *parent)
    : QObject(parent)
{
}

/** Returns the primary key for this QDjangoModel.
 */
QVariant NOrmModel::pk() const
{
    const NOrmMetaModel metaModel = NOrm::metaModel(metaObject()->className());
    return property(metaModel.primaryKey());
}

/** Sets the primary key for this QDjangoModel.
 *
 * \param pk
 */
void NOrmModel::setPk(const QVariant &pk)
{
    const NOrmMetaModel metaModel = NOrm::metaModel(metaObject()->className());
    setProperty(metaModel.primaryKey(), pk);
}

/** Retrieves the QDjangoModel pointed to by the given foreign-key.
 *
 * \param name
 */
QObject *NOrmModel::foreignKey(const char *name) const
{
    const NOrmMetaModel metaModel = NOrm::metaModel(metaObject()->className());
    return metaModel.foreignKey(this, name);
}

/** Sets the QDjangoModel pointed to by the given foreign-key.
 *
 * \param name
 * \param value
 *
 * \note The QDjangoModel will not take ownership of the given \c value.
 */
void NOrmModel::setForeignKey(const char *name, QObject *value)
{
    const NOrmMetaModel metaModel = NOrm::metaModel(metaObject()->className());
    metaModel.setForeignKey(this, name, value);
}

/** Deletes the QDjangoModel from the database.
 *
 * \return true if deletion succeeded, false otherwise
 */
bool NOrmModel::remove()
{
    const NOrmMetaModel metaModel = NOrm::metaModel(metaObject()->className());
    return metaModel.remove(this);
}

/** Saves the QDjangoModel to the database.
 *
 * \return true if saving succeeded, false otherwise
 */
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

