#include "MainControlWindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainControlWindow w;
    w.show();
    return a.exec();
}