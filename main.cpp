#include <QApplication>
#include <QIcon>
#include <QLocale>
#include <QTranslator>

#include "mainwindow.hpp"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "PerformanceMeasurer_" + QLocale(locale).name();
        if (translator.load("translations/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    MainWindow w;
    w.setWindowIcon(QIcon("assets/icon.ico"));
    w.show();
    return a.exec();
}
