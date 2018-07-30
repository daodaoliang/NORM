#include "testmodel.h"

void TestTable::setTestFieldString(const QString &testFieldString)
{
    mTestFieldString = testFieldString;
}

TestTable::TestTable(QObject *parent)
    : NOrmModel(parent)
{
}

QString TestTable::testFieldString() const
{
    return mTestFieldString;
}

bool TestTable::testFieldBool() const
{
    return mTestFieldBool;
}

void TestTable::setTestFieldBool(bool testFieldBool)
{
    mTestFieldBool = testFieldBool;
}

QDate TestTable::testFieldDate() const
{
    return mTestFieldDate;
}

void TestTable::setTestFieldDate(const QDate &testFieldDate)
{
    mTestFieldDate = testFieldDate;
}

QTime TestTable::testFieldTime() const
{
    return mTestFieldTime;
}

void TestTable::setTestFieldTime(const QTime &testFieldTime)
{
    mTestFieldTime = testFieldTime;
}

QDateTime TestTable::testFieldDateTime() const
{
    return mTestFieldDateTime;
}

void TestTable::setTestFieldDateTime(const QDateTime &testFieldDateTime)
{
    mTestFieldDateTime = testFieldDateTime;
}

QByteArray TestTable::testFieldByteArray() const
{
    return mTestFieldByteArray;
}

void TestTable::setTestFieldByteArray(const QByteArray &testFieldByteArray)
{
    mTestFieldByteArray = testFieldByteArray;
}

double TestTable::testFieldDouble() const
{
    return mTestFieldDouble;
}

void TestTable::setTestFieldDouble(double testFieldDouble)
{
    mTestFieldDouble = testFieldDouble;
}

quint32 TestTable::testFieldInt() const
{
    return mTestFieldInt;
}

void TestTable::setTestFieldInt(const quint32 &testFieldInt)
{
    mTestFieldInt = testFieldInt;
}

QStringList TestTable::testStringList() const
{
    return mTestStringList;
}

void TestTable::setTestStringList(const QStringList &testStringList)
{
    mTestStringList = testStringList;
}
