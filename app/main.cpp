#include <QApplication>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    qh::app::MainWindow window;
    window.show();
    return app.exec();
}
