#include <QCoreApplication>
#include "QDjango.h"
#include <QSqlDatabase>
#include <QDebug>
#include "testmodel.h"

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
    // 初始化测试环境
    bool ret = initTestEnv();
    qDebug()<<QObject::tr("测试环境初始化:%1").arg(ret?"成功":"失败");
    // 注册模型
    QDjango::registerModel<TestTable>();
    // 创建表结构
    QDjango::dropTables();
    ret = QDjango::createTables();
    qDebug()<<QObject::tr("创建数据库表结构:%1").arg(ret?"成功":"失败");
    // 增加测试数据
    TestTable test_case;
    test_case.setTestFieldBool(false);
    test_case.setTestFieldByteArray(QByteArray(10, 'a'));
    test_case.setTestFieldDate(QDate::currentDate());
    test_case.setTestFieldDateTime(QDateTime::currentDateTime());
    test_case.setTestFieldDouble(12.345678);
    test_case.setTestFieldString("我就是一段测试数据，我是娜美");
    test_case.setTestFieldTime(QTime::currentTime());
    test_case.save();

    qDebug()<<"************************测试用例结束**********************************";
    return a.exec();
}
