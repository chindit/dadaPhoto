#-------------------------------------------------
#
# Project created by QtCreator 2014-08-13T19:04:28
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dadaPhotos
TEMPLATE = app

LIBS += -lexiv2

SOURCES += main.cpp\
        dadaphoto.cpp

HEADERS  += dadaphoto.h

FORMS    += dadaphoto.ui

RESOURCES += \
    ressources.qrc
