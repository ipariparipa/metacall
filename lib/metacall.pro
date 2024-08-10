TEMPLATE = lib
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

#QMAKE_CXXFLAGS += -std=c++2a

CONFIG += staticlib

SOURCES += \
        application.cpp \
        connection.cpp \
        notifier.cpp \
        object.cpp \
        timer.cpp

HEADERS += \
    include/metacall/application.h \
    include/metacall/common.h \
    include/metacall/connection.h \
    include/metacall/delegate.h \
    include/metacall/object.h \
    include/metacall/timer.h \
    notifier.h

INCLUDEPATH += include
