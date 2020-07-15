#ifndef NORM_H
#define NORM_H

#include "NOrmMetaModel.h"

class QObject;
class QSqlDatabase;
class QSqlQuery;
class QString;

/**
 * @brief The NOrm class
 * NORM的接口
 */
class NOrm
{
public:
    /**
     * @brief createTables 创建数据表
     * @return 创建操作的结果
     */
    static bool createTables();

    /**
     * @brief dropTables 删除数据表
     * @return 删除操作的结果
     */
    static bool dropTables();

    /**
     * @brief database 当前链接的数据库
     * @return 数据库信息
     */
    static QSqlDatabase database();

    /**
     * @brief setDatabase 设置当前需要连接的数据库
     * @param database 数据库信息
     * @return 设置操作结果
     */
    static bool setDatabase(QSqlDatabase database);

    /**
     * @brief isDebugEnabled 是否是调试模式
     * @return true or false
     */
    static bool isDebugEnabled();

    /**
     * @brief setDebugEnabled 设置组件是否运行在调试模式
     * @param enabled true or false
     */
    static void setDebugEnabled(bool enabled);

    template <class T>
    /**
     * @brief registerModel 注册模型(模板)
     * @return 模型结构体
     */
    static NOrmMetaModel registerModel();

    /**
     * @brief metaModel 模型数据
     * @param name 模型的名字
     * @return 封装好的模型对象
     */
    static NOrmMetaModel metaModel(const char *name);
	static QStack<NOrmMetaModel> metaModels();

private:
    /**
     * @brief registerModel 注册模型
     * @param meta 模型对象
     * @return 封装好的模型对象
     */
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
