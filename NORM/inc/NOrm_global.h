#ifndef NORM_GLOBAL_H
#define NORM_GLOBAL_H
/*
 * 描述: NORM api 库接口定义
 * 作者: daodaoliang@yeah.net
 * 时间: 2021-07-16
 */

#if defined(NORM_LIBRARY)
#  define NORMSHARED_EXPORT Q_DECL_EXPORT
#else
#  define NORMSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // NORM_GLOBAL_H
