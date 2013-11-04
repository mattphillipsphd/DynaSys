#-------------------------------------------------
#
# Project created by QtCreator 2013-08-03T00:26:06
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = DynaSys
TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS += -gdwarf-3


SOURCES += main.cpp\
        gui/mainwindow.cpp \
    models/parammodel.cpp \
    gui/variablegui.cpp \
    file/sysfileout.cpp \
    file/sysfilein.cpp \
    models/conditionmodel.cpp \
    gui/comboboxdelegate.cpp

HEADERS  += gui/mainwindow.h \
    models/parammodel.h \
    gui/variablegui.h \
    file/sysfileout.h \
    file/sysfilein.h \
    models/conditionmodel.h \
    gui/comboboxdelegate.h

FORMS    += forms/mainwindow.ui \
    forms/variablegui.ui

win32 {
    QWT_DIR         = C:/Users/matt/Libraries/qwt-6.1.0
    MUPARSER_DIR    = C:/Users/matt/Libraries/muparser_v2_2_3
} else {
    QWT_DIR         = /home/matt/Libraries/qwt-6.1.0
    MUPARSER_DIR    = /home/matt/Libraries/muparser
}

#CONFIG(debug, debug|release) {
#    INCLUDEPATH += /home/matt/qwt-6.1.0-debug/src
#    LIBS += -L/home/matt/qwt-6.1.0-debug/lib -lqwt
#} else {
#    INCLUDEPATH += /home/matt/qwt-6.1.0/src
#    LIBS += -L/home/matt/qwt-6.1.0/lib -lqwt
#}

INCLUDEPATH += shared/boost \
                $$MUPARSER_DIR/include \
                $$QWT_DIR/src

win32 {
    LIBS += C:/Users/matt/Libraries/muparser_v2_2_3/lib/muParser.lib
    #LIBS += -L$$MUPARSER_DIR/lib/muParser.lib \
    LIBS +=        -L$$QWT_DIR/lib/ -lqwt
} else {
    LIBS += -L$$MUPARSER_DIR/lib -lmuparser \
            -L$$QWT_DIR/lib -lqwt
}

#For Ubuntu on the home desktop
#INCLUDEPATH += /home/matt/openblas/openblas-0.2.6-nothread/lapack-3.4.2/lapacke/include
#LIBS += /home/matt/openblas/openblas-0.2.6-nothread/libopenblas.a -lgfortran

