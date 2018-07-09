#ifndef NORM_H
#define NORM_H

#include "NOrmMetaModel.h"
#include "NOrm_global.h"

class QObject;
class QSqlDatabase;
class QSqlQuery;
class QString;

class NORMSHARED_EXPORT NOrm
{
public:
    static bool createTables();
    static bool dropTables();

    static QSqlDatabase database();
    static bool setDatabase(QSqlDatabase database);

    static bool isDebugEnabled();
    static void setDebugEnabled(bool enabled);

    template <class T>
    static NOrmMetaModel registerModel();
    static NOrmMetaModel metaModel(const char *name);

private:
    static NOrmMetaModel registerModel(const QMetaObject *meta);

    friend class NOrmCompiler;
    friend class NOrmModel;
    friend class NOrmMetaModel;
    friend class NOrmSetPrivate;
};

template <class T>
NOrmMetaModel NOrm::registerModel()
{
    return registerModel(&T::staticMetaObject);
}

#endif
