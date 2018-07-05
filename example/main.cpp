#include <QCoreApplication>
#include "QDjango.h"
#include <QSqlDatabase>
#include <QDebug>
#include "testmodel.h"
#include "QDjangoQuerySet.h"

bool initTestEnv(){
    // 设置调试模式
    QDjango::setDebugEnabled(true);
    // 设置数据库的初始信息
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setDatabaseName("test");
    db.setHostName("127.0.0.1");
    db.setPassword("");
    db.setPort(3306);
    db.setUserName("root");
    if(db.open()){
        QDjango::setDatabase(db);
        return true;
    }
    return false;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    qDebug()<<"************************测试用例开始**********************************";

    // workflow 001 --> 初始化测试环境
    bool ret = initTestEnv();
    qDebug()<<QObject::tr("测试环境初始化:%1").arg(ret?"成功":"失败");

    // workflow 002 --> 注册模型
    QDjango::registerModel<TestTable>();

    // workflow 004 --> 初始化数据表
    QStringList tableList = QDjango::database().tables();
    if(tableList.contains("testtable")){
        QDjango::dropTables();
    }
    ret = QDjango::createTables();
    qDebug()<<QObject::tr("创建数据库表结构:%1").arg(ret?"成功":"失败");

    // workflow 005 --> 数据增加测试
    for(int index=0;index!=10;++index){
        TestTable test_case;
        test_case.setTestFieldBool(false);
        test_case.setTestFieldByteArray(QByteArray(10, 'a'));
        test_case.setTestFieldDate(QDate::currentDate());
        test_case.setTestFieldDateTime(QDateTime::currentDateTime());
        test_case.setTestFieldDouble(12.345678);
        test_case.setTestFieldString("我就是一段测试数据，我是娜美");
        test_case.setTestFieldTime(QTime::currentTime());
        test_case.save();
        qDebug()<<"增加了一条新的数据记录";
    }

    // 数据查询测试 --> 0061 count 数据条数
    QDjangoQuerySet<TestTable> testTables_case;
    qDebug() << QObject::tr("当前数据库共有数据:%1条").arg(testTables_case.count());

    // 数据查询测试 --> 0062 单where 查询一条数据
    TestTable* testRecord;
    testRecord = testTables_case.get(QDjangoWhere("id", QDjangoWhere::Equals, 1));
    qDebug()<<QObject::tr("查询结果: %1").arg(testRecord->testFieldDouble());
    qDebug()<<QObject::tr("查询结果: %1").arg(testRecord->testFieldString());
    qDebug()<<QObject::tr("查询结果: %1").arg(testRecord->testFieldDateTime().toString());

    // 数据查询测试 --> 0063 单where 查询多条数据
    QDjangoQuerySet<TestTable> queryMany;
    QDjangoQuerySet<TestTable> rets = queryMany.filter(QDjangoWhere("id", QDjangoWhere::GreaterOrEquals, 1));
    for(int tmpIndex=0;tmpIndex!=rets.size();++tmpIndex){
        qDebug()<<QObject::tr("查询数据的结果主键:%1").arg(rets.at(tmpIndex)->pk().toString());
    }

    // 数据查询测试 --> 0064 多where查询 与查询
    rets = queryMany.filter(QDjangoWhere("id", QDjangoWhere::GreaterOrEquals, 1) && QDjangoWhere("id", QDjangoWhere::LessOrEquals, 5));
    for(int tmpIndex=0;tmpIndex!=rets.size();++tmpIndex){
        qDebug()<<QObject::tr("查询数据的结果主键:%1").arg(rets.at(tmpIndex)->pk().toString());
    }

    // 数据查询测试 --> 0065 多where查询 或查询
    rets = queryMany.filter(QDjangoWhere("id", QDjangoWhere::GreaterOrEquals, 4) || QDjangoWhere("id", QDjangoWhere::LessOrEquals, 2));
    for(int tmpIndex=0;tmpIndex!=rets.size();++tmpIndex){
        qDebug()<<QObject::tr("查询数据的结果主键:%1").arg(rets.at(tmpIndex)->pk().toString());
    }

    qDebug()<<"************************测试用例结束**********************************";
    return a.exec();
}
