#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDateTime>
#include <QDir>
#include <QMainWindow>
#include <QMessageBox>
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
    QTimer m_timer; // To set datetime.
    Database m_db;

    void setLabel();
    void setDateTime();
    void setTodaysCalls();
    bool shouldUpdate();
public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
private slots:
    void error(const QString& message);
    void newCall();
    void saveCalls();
};

#endif // MAINWINDOW_H
