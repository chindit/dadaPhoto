#-------------------------------------------------
#
# Project created by QtCreator 2014-08-13T19:04:28
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dadaPhotos
TEMPLATE = app

INCLUDEPATH += 'exiv2/mingw/include/'

#LIBS += 'C:\Users\David\Documents\dist\2015\x64\dll\Release\bin\exiv2.dll'

SOURCES += main.cpp\
        dadaphoto.cpp\
        lib/easyexif/exif.cpp

HEADERS  += dadaphoto.h\
            lib/easyexif/exif.h

FORMS    += dadaphoto.ui

RESOURCES += \
    ressources.qrc
