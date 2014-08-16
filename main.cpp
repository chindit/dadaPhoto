#include "dadaphoto.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    dadaPhoto instance;
    instance.show();

    return app.exec();
}
