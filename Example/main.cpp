#include <QCoreApplication>
#include "NOrm.h"
#include <QSqlDatabase>
#include <QDebug>
#include <QtTest/QTest>
#include "testmodel.h"
#include "NOrmQuerySet.h"
#include <QUuid>
#include <QJsonDocument>
#include <QTime>

bool initTestEnv() {
    // 数据库基本信息
    QString db_host = "127.0.0.1";
    QString db_user = "root";
    QString db_pwd = "sduept";
    QString db_name = "test";
    qint32 db_port = 3306;

    // 设置调试模式（调试模式下会进行详细的日志输出）
    NOrm::setDebugEnabled(true);

    // 设置数据库的初始信息(数据库链接名字用来唯一识别数据库链接)
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "test_");
    db.setHostName(db_host);
    db.setPassword(db_pwd);
    db.setPort(db_port);
    db.setUserName(db_user);
    db.setDatabaseName(db_name);
    bool ret = NOrm::setDatabase(db);
    if (NOrm::isDebugEnabled()) {
        qDebug() << QObject::tr("数据库设置:%1").arg(ret ? "成功" : "失败");
    }
    return ret;
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    qDebug() << "************************测试用例开始**********************************";
    QTime mCountTime;
    mCountTime.start();

    // workflow 001 --> 初始化测试环境
    bool ret = initTestEnv();
    qDebug() << QObject::tr("测试环境初始化:%1").arg(ret ? "成功" : "失败");
    if (!ret) {
        return -1;
    }

    // workflow 002 --> 注册模型
    NOrm::registerModel<TestTable>();

    // workflow 004 --> 初始化数据表(获取数据表数据表存在则进行删除)
    QStringList tableList = NOrm::database().tables();
    if (tableList.contains("testtable")) {
        NOrm::dropTables();
    }
    ret = NOrm::createTables();
    qDebug() << QObject::tr("创建数据库表结构:%1").arg(ret ? "成功" : "失败");
    if (!ret) {
        return -1;
    }

    // workflow 005 --> 数据增加测试（100条数据）
    NOrm::database().transaction();
    for (int index = 0; index != 100; ++index) {
        TestTable test_case;
        test_case.setTestFieldBool(true);
        test_case.setTestFieldByteArray(QByteArray(10, 'a'));
        test_case.setTestFieldDate(QDate::currentDate());
        test_case.setTestFieldDateTime(QDateTime::currentDateTime());
        test_case.setTestFieldDouble(12.345678);
        test_case.setTestFieldString(QUuid::createUuid().toString().replace("{", "").replace("}", ""));
        test_case.setTestFieldTime(QTime::currentTime());
        test_case.setTestFieldInt(static_cast<quint32>(index));
        test_case.setTestStringList(QStringList() << "123" << "234");
        test_case.save();
        qDebug() << "增加了一条新的数据记录";
    }
    NOrm::database().commit();

    // 数据查询测试 --> 0061 count 数据条数
    NOrmQuerySet<TestTable> testTables_case;
    qDebug() << QObject::tr("当前数据库共有数据:%1条").arg(testTables_case.count());

    // 数据查询测试 --> 0062 单where 查询单条数据
    NOrmQuerySet<TestTable> queryMany;
    TestTable* rets_single = queryMany.get(NOrmWhere("testFieldBool", NOrmWhere::GreaterOrEquals, true));

    if(rets_single){
        // 打印测试
        qDebug() << QObject::tr("查询数据的结果主键:%1").arg(rets_single->pk().toString());
    }

    // 数据查询测试 --> 0063 多 where查询
    NOrmQuerySet<TestTable> rets = queryMany.filter(NOrmWhere("testFieldInt", NOrmWhere::GreaterOrEquals, 1) && NOrmWhere("testFieldInt", NOrmWhere::LessOrEquals, 5));
    for (int tmpIndex = 0; tmpIndex != rets.size(); ++tmpIndex) {
        // 读取这一行数据
        auto rowData = rets.at(tmpIndex);

        // 打印测试
        qDebug() << QObject::tr("查询数据的结果主键:%1").arg(rowData->pk().toString());

        // 内存释放
        delete rowData;
    }

    // 数据查询测试 --> 0064 多where查询
    rets = queryMany.filter(NOrmWhere("testFieldInt", NOrmWhere::GreaterOrEquals, 4) || NOrmWhere("testFieldInt", NOrmWhere::LessOrEquals, 2));
    for (int tmpIndex = 0; tmpIndex != rets.size(); ++tmpIndex) {
        // 读取这一行数据
        auto rowData = rets.at(tmpIndex);

        // 打印测试
        qDebug() << QObject::tr("查询数据的结果主键:%1").arg(rowData->pk().toString());

        // 内存释放
        delete rowData;
    }

    qDebug() << "************************测试用例结束**********************************";
    qDebug() << " 测试用例花费时间:" << mCountTime.elapsed() << " 毫秒";
    return 0;
}
