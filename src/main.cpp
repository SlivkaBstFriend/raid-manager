#include <QApplication>
#include <QStyleFactory>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("RAID Manager");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("raid-manager");

    // Use Fusion style for consistent cross-distro look
    app.setStyle(QStyleFactory::create("Fusion"));

    MainWindow w;
    w.show();
    return app.exec();
}
