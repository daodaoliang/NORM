QT -= gui
QT += sql

CONFIG += c++11 console
CONFIG -= app_bundle
TARGET = QtORM_Example
DEFINES += QT_DEPRECATED_WARNINGS

# 引入第三方库
include($$PWD/../NORM/NORM_inc.pri)

SOURCES += main.cpp \
    testmodel.cpp

HEADERS += \
    testmodel.h
