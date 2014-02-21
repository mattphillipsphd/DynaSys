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

OBJECTS_DIR = obj
MOC_DIR = moc
UI_DIR = ui

SOURCES += main.cpp\
        gui/mainwindow.cpp \
    models/parammodel.cpp \
    gui/variablegui.cpp \
    file/sysfileout.cpp \
    file/sysfilein.cpp \
    models/conditionmodel.cpp \
    gui/comboboxdelegate.cpp \
    gui/aboutgui.cpp \
    memrep/parsermgr.cpp \
    gui/columnview.cpp \
    memrep/input.cpp \
    models/variablemodel.cpp \
    models/differentialmodel.cpp \
    models/initialcondmodel.cpp \
    globals/globals.cpp \
    models/parammodelbase.cpp \
    models/tpvtablemodel.cpp \
    gui/checkboxdelegate.cpp

HEADERS  += gui/mainwindow.h \
    models/parammodel.h \
    gui/variablegui.h \
    file/sysfileout.h \
    file/sysfilein.h \
    models/conditionmodel.h \
    gui/comboboxdelegate.h \
    gui/aboutgui.h \
    memrep/parsermgr.h \
    gui/columnview.h \
    memrep/input.h \
    models/variablemodel.h \
    models/differentialmodel.h \
    models/initialcondmodel.h \
    globals/globals.h \
    models/parammodelbase.h \
    models/tpvtablemodel.h \
    gui/checkboxdelegate.h

FORMS    += forms/mainwindow.ui \
    forms/aboutgui.ui \
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
                $$QWT_DIR/src \
                .

win32 {
    CONFIG(debug, debug|release) {
        LIBS += C:/Users/matt/Libraries/muparser_v2_2_3/lib/muparserd.lib
        LIBS +=        -L$$QWT_DIR/lib/ -lqwtd
    } else {
        LIBS += C:/Users/matt/Libraries/muparser_v2_2_3/lib/muParser.lib
        LIBS +=        -L$$QWT_DIR/lib/ -lqwt
    }
} else {
    LIBS += -L$$MUPARSER_DIR/lib -lmuparser \
            -L$$QWT_DIR/lib -lqwt
}

#For Ubuntu on the home desktop
#INCLUDEPATH += /home/matt/openblas/openblas-0.2.6-nothread/lapack-3.4.2/lapacke/include
#LIBS += /home/matt/openblas/openblas-0.2.6-nothread/libopenblas.a -lgfortran

