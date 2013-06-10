#include "ui/MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    Qoper::MainWindow window;
    window.show();

    return application.exec();
}
