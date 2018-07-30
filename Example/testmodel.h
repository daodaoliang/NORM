#ifndef TESTMODEL_H
#define TESTMODEL_H
/**
 * 作者: daodaoliang
 * 时间: 2018年7月5日
 * 版本: 1.0.1.0
 * 邮箱: daodaoliang@yeah.net
 */

#include <QObject>
#include "NOrmModel.h"
#include <QDateTime>
#include <QByteArray>

class TestTable: public NOrmModel {
    Q_OBJECT
    Q_PROPERTY(QString testFieldString READ testFieldString WRITE setTestFieldString)
    Q_PROPERTY(bool testFieldBool READ testFieldBool WRITE setTestFieldBool)
    Q_PROPERTY(QDate testFieldDate READ testFieldDate WRITE setTestFieldDate)
    Q_PROPERTY(QTime testFieldTime READ testFieldTime WRITE setTestFieldTime)
    Q_PROPERTY(QDateTime testFieldDateTime READ testFieldDateTime WRITE setTestFieldDateTime)
    Q_PROPERTY(QByteArray testFieldByteArray READ testFieldByteArray WRITE setTestFieldByteArray)
    Q_PROPERTY(double testFieldDouble READ testFieldDouble WRITE setTestFieldDouble)
    Q_PROPERTY(int testFieldInt READ testFieldInt WRITE setTestFieldInt)
    Q_PROPERTY(QStringList testStringList READ testStringList WRITE setTestStringList)
    Q_CLASSINFO("testFieldString","primary_key=true max_length=45")

public:
    TestTable(QObject *parent=0);

public:
    /***************** setter && getter ********************/
    QString testFieldString() const;
    void setTestFieldString(const QString &testFieldString);

    bool testFieldBool() const;
    void setTestFieldBool(bool testFieldBool);

    QDate testFieldDate() const;
    void setTestFieldDate(const QDate &testFieldDate);

    QTime testFieldTime() const;
    void setTestFieldTime(const QTime &testFieldTime);

    QDateTime testFieldDateTime() const;
    void setTestFieldDateTime(const QDateTime &testFieldDateTime);

    QByteArray testFieldByteArray() const;
    void setTestFieldByteArray(const QByteArray &testFieldByteArray);

    double testFieldDouble() const;
    void setTestFieldDouble(double testFieldDouble);

    quint32 testFieldInt() const;
    void setTestFieldInt(const quint32 &testFieldInt);

    QStringList testStringList() const;
    void setTestStringList(const QStringList &testStringList);

private:
    // string 类型的字段
    QString mTestFieldString;
    // bool 类型的字段
    bool mTestFieldBool;
    // date 类型的字段
    QDate mTestFieldDate;
    // time 类型的字段
    QTime mTestFieldTime;
    // datetime 类型的字段
    QDateTime mTestFieldDateTime;
    // ByteArray 类型的字段
    QByteArray mTestFieldByteArray;
    // Double 类型的字段
    double mTestFieldDouble;
    // int 类型的字段
    int mTestFieldInt;
    // stringlist 类型的字段
    QStringList mTestStringList;
};


#endif // TESTMODEL_H
