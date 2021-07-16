#ifndef NORM_WHERE_P_H
#define NORM_WHERE_P_H

#include <QSharedData>
#include "NOrmWhere.h"

class NOrmWherePrivate : public QSharedData
{
public:
    static QString operationToString(NOrmWhere::Operation operation);

    enum Combine
    {
        NoCombine,
        AndCombine,
        OrCombine
    };
    static QString combineToString(Combine combine);

    NOrmWherePrivate();

    QString key;
    NOrmWhere::Operation operation;
    QVariant data;

    QList<NOrmWhere> children;
    Combine combine;
    bool negate;
};

#endif
