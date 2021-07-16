QT -= gui
QT += sql

CONFIG += c++11 console
CONFIG -= app_bundle
TARGET = orm_example
DEFINES += QT_DEPRECATED_WARNINGS

# 引入第三方库
include($$PWD/../NORM/NORM_inc.pri)

# include path
INCLUDEPATH += $$PWD/../NORM/inc/

# include sources
SOURCES += main.cpp \
    testmodel.cpp

HEADERS += \
    testmodel.h

# 输出定义
win32{
    CONFIG += debug_and_release
    CONFIG(release, debug|release) {
            target_path = ./build_/dist
        } else {
            target_path = ./build_/debug
    }
    DESTDIR = $$PWD/../bin
    MOC_DIR = $$target_path/moc
    RCC_DIR = $$target_path/rcc
    OBJECTS_DIR = $$target_path/obj
}

unix{
    CONFIG += debug_and_release
    CONFIG(release, debug|release) {
            target_path = ./build_/dist
        } else {
            target_path = ./build_/debug
    }
    DESTDIR = $$PWD/../bin/
    MOC_DIR = $$target_path/moc
    RCC_DIR = $$target_path/rcc
    OBJECTS_DIR = $$target_path/obj
}
