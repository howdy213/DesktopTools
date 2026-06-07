#include "modifydesktop.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    ModifyDesktop window;
    window.show();
    return app.exec();
}
