#include "dadaphoto.h"
#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QLibraryInfo>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QTranslator frenchTranslator;
    if (frenchTranslator.load("qt_" + QLocale::system().name())){
        app.installTranslator(&frenchTranslator);
    }

    dadaPhoto instance;
    instance.show();

    return app.exec();
}
