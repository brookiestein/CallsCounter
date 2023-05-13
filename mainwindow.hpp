#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QPixmap>
#include <QRect>
#include <QScreen>
#include <QSqlRecord>
#include <QTimer>

#include "database.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
    Ui::MainWindow *m_ui;
    int m_calls;
    QString m_datetime;
    QString m_username;
    QTimer m_lockerTimer;
    QTimer m_saverTimer;
    QTimer m_saverTimer1s;
    QTimer m_datetimeTimer;
    bool m_lock;
    Database m_db;

    void setLabel();
    void setDateTime();
    void setTodaysCalls();
    bool shouldUpdate();
    QPixmap takeScreenShot();
public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
private slots:
    void about(bool checked);
    void unlockNewButton();
    void error(const QString& message);
    void newCall();
    void removeCall();
    void saveCalls();
    void saveScreenShot();
    void setRemainingTimeLabel();
};

#endif // MAINWINDOW_H
