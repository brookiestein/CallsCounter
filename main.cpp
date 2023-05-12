#include <QApplication>
#include <QIcon>

#include "mainwindow.hpp"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowIcon(QIcon("assets/icon.ico"));
    w.show();
    return a.exec();
}
