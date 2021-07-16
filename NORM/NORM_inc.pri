#-------------------------------------------------
#
# Project created by daodaoliang 2018-07-5T11:08:28
#
#-------------------------------------------------

INCLUDEPATH += $$PWD\inc

win32{
    LIBS += -L$$PWD/../bin/ -lnorm_sql
    DEPENDPATH += $$PWD/../bin
}

unix{
    LIBS += -L$$PWD/../bin/ -lnorm_sql
    DEPENDPATH += $$PWD/../bin
}

