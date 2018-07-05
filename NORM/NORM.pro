QT -= gui
QT += sql

TEMPLATE = lib
CONFIG += shared
TARGET = NORM
DEFINES += NORM_LIBRARY
CONFIG += c++11

# 版本号
win32{
    RC_FILE += ./NORM_resource.rc
}
unix{
    VERSION = 1.0.1
}

# 源码信息
include($$PWD/NORM_src.pri)
INCLUDEPATH += ./inc/ \

# 输出定义
win32{
    CONFIG += debug_and_release
    CONFIG(release, debug|release) {
            target_path = ./build_/dist
        } else {
            target_path = ./build_/debug
    }
    DESTDIR = ../bin
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
    DESTDIR = ../bin
    MOC_DIR = $$target_path/moc
    RCC_DIR = $$target_path/rcc
    OBJECTS_DIR = $$target_path/obj
}

# 打印信息
message(Qt version: $$[QT_VERSION])
message(Qt is installed in $$[QT_INSTALL_PREFIX])
message(the NORM will create in folder: $$target_path)
