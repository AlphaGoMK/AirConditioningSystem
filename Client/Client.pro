#-------------------------------------------------
#
# Project created by QtCreator 2018-05-15T18:48:04
#
#-------------------------------------------------

QT       += core gui
QT       += network
QT       += sql
LIBS += -lpthread libwsock32 libws2_32
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Client
TEMPLATE = app


SOURCES += main.cpp\
    client.cpp \
    initwindow.cpp

HEADERS  += \
    client.h \
    initwindow.h

FORMS    += widget.ui \
    client.ui \
    initwindow.ui
