#include <QApplication>

#include "NordicCentral.h"
#include "Window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    NordicCentral nordicCentral;

    Window window(nordicCentral);
    window.show();

    return app.exec();
}
