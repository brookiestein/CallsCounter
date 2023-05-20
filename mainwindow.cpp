#include "mainwindow.hpp"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
    , m_calls(0)
    , m_db()
    , m_lock(false)
{
    m_ui->setupUi(this);
    m_ui->lastSavedLabel->setText("");
    m_ui->lastCallLabel->setText("");
    m_ui->currentBarSetValue->setText("");

    connect(&m_db, &Database::error, this, &MainWindow::error);
    connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(m_ui->newButton, &QPushButton::clicked, this, &MainWindow::newCall);
    connect(m_ui->removeButton, &QPushButton::clicked, this, &MainWindow::removeCall);
    connect(m_ui->saveButton, &QPushButton::clicked, this, &MainWindow::saveCalls);
    connect(&m_saverTimer, &QTimer::timeout, this, &MainWindow::saveCalls);
    connect(&m_saverTimer1s, &QTimer::timeout, this, &MainWindow::setRemainingTimeLabel);
    connect(&m_datetimeTimer, &QTimer::timeout, this, &MainWindow::setDateTime);
    connect(&m_lockerTimer, &QTimer::timeout, this, &MainWindow::unlockNewButton);

    m_username = qgetenv("USER");
    if (m_username.isEmpty())
        m_username = qgetenv("USERNAME");
    m_ui->usernameLabel->setText(tr("User: ") + m_username);

    m_chart = new Chart(m_db, m_username, m_ui->centralwidget);
    m_chart->setGeometry(QRect(280, 30, 675, 381));
    connect(m_chart, &Chart::updateHoveredLabel, this, &MainWindow::updateHoveredLabel);

    setDateTime();
    setTodaysCalls();

#ifdef QT_DEBUG
    m_lockerTimer.setInterval(500);
#else
    m_lockerTimer.setInterval(3'000);
#endif
    m_saverTimer.setInterval(300'000); // 5 minutes.
    m_saverTimer1s.setInterval(1'000);
    m_datetimeTimer.setInterval(1'000);
    m_datetimeTimer.start();
    m_saverTimer.start();
    m_saverTimer1s.start();

    m_ui->newButton->setShortcut(Qt::CTRL | Qt::Key_A);
    m_ui->removeButton->setShortcut(Qt::CTRL | Qt::Key_R);
    m_ui->saveButton->setShortcut(Qt::CTRL | Qt::Key_S);
}

MainWindow::~MainWindow()
{
    delete m_ui;
}

void MainWindow::setLabel(bool justMessageLabel, const QString& prefix = "")
{
    m_ui->messageLabel->setText(tr("You've registered %1 %2 today.")
                                    .arg(QString::number(m_calls), m_calls == 1 ? tr("call") : tr("calls")));

    if (not justMessageLabel) {
        auto time { QDateTime::currentDateTime().toString("hh:mm:ss") };
        m_ui->lastCallLabel->setText(tr("%1 call at %2").arg(prefix, time));
    }
}

void MainWindow::setDateTime()
{
    auto dt { QDateTime::currentDateTime() };
    auto day { dt.toString("ddd") };
    m_datetime = dt.toString("dd-MM-yyyy");
    auto time = dt.toString("hh:mm:ss");
    m_ui->datetimeLabel->setText(tr("Date: %1, %2 %3").arg(day, m_datetime, time));
}

void MainWindow::setTodaysCalls()
{
    if (not m_db.open())
        return;
    // shouldUpdate() will help us because it checks if there's at least one record in the database.
    // If there aren't any, we'll get 'not positioned on a valid record' error message, so to avoid this:
    if (not shouldUpdate()) {
        m_db.close();
        setLabel(true);
        return;
    }

    auto statement = QString("SELECT id, calls FROM Calls WHERE date='%1' ORDER BY id DESC LIMIT 1").arg(m_datetime);
    if (not m_db.exec(statement)) {
        m_db.close();
        return;
    }

    auto record { m_db.record() };
    int index { record.indexOf("calls") };
    m_calls = m_db.value(index).toInt();
    if (m_calls == -1)
        m_calls = 0;
    else
        m_chart->setValue(dayOfWeek(), m_calls);

    setLabel(true);
    m_db.close();
}

void MainWindow::error(const QString& message)
{
    QMessageBox::critical(this, PROGRAM_NAME, message);
}

bool MainWindow::shouldUpdate()
{
    auto statement = QString("SELECT COUNT(id) AS fields FROM Calls WHERE username='%1' AND date='%2'")
                         .arg(m_username, m_datetime);
    if (not m_db.exec(statement))
        return false;
    auto record { m_db.record() };
    int index { record.indexOf("fields") };
    int fields { m_db.value(index).toInt() };
    if (fields == 0)
        return false;
    return true;
}

int MainWindow::dayOfWeek()
{
    QDate date { QDate::currentDate() };
    return date.dayOfWeek();
}

void MainWindow::about(bool checked)
{
    Q_UNUSED(checked);
    auto message = tr("<p>Version: %1</p>\
<p>Author: %2</p>\
<a href=\"%3\">This software</a><br>\
<a href=\"%4\">Other projects</a>").arg(VERSION, AUTHOR, THIS_PROGRAM_URL, OTHER_PROJECTS);
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(PROGRAM_NAME);
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText(message);
    msgBox.exec();
}

void MainWindow::unlockNewButton()
{
    m_lock = false;
}

void MainWindow::newCall()
{
    if (m_lock) {
        QMessageBox::warning(this, PROGRAM_NAME, tr("You have to wait a couple of secs to register a new call."));
        return;
    } else {
        m_lock = true;
        m_lockerTimer.start();
    }

    ++m_calls;
    setLabel(false, "Last registered");
    m_chart->setValue(dayOfWeek() - 1, m_calls);
}

void MainWindow::removeCall()
{
    if (m_lock) {
        QMessageBox::warning(this, PROGRAM_NAME, tr("You have to wait a couple of secs to remove a call."));
        return;
    } else {
        m_lock = true;
        m_lockerTimer.start();
    }

    if (m_calls == 0) {
        error(tr("Registered calls cannot be below zero."));
        return;
    }

    --m_calls;
    setLabel(false, "Last removed");
    m_chart->setValue(dayOfWeek() - 1, m_calls);
}

void MainWindow::saveCalls()
{
    if (not m_db.open())
        return;

    bool update { shouldUpdate() };
    QString statement;

    if (update) {
        // I prefer to use the id in the WHERE clause, but we have no problem since date cannot be duplicated.
        statement = QString("UPDATE Calls SET calls='%1' WHERE date='%2' AND username='%3'")
                        .arg(QString::number(m_calls), m_datetime, m_username);
    } else {
        statement = QString("INSERT INTO Calls (calls, date, username) VALUES ('%1', '%2', '%3')")
                        .arg(QString::number(m_calls), m_datetime, m_username);
    }

    if (not m_db.exec(statement)) {
        m_db.close();
        return;
    }

    m_db.close();

    if (sender() == m_ui->saveButton) {
        QMessageBox::information(this, PROGRAM_NAME, tr("Calls have been saved."));
    }

    m_saverTimer.start();
    auto time { QDateTime::currentDateTime().toString("hh:mm:ss") };
    m_ui->lastSavedLabel->setText(tr("Last saved at %1").arg(time));
}

void MainWindow::setRemainingTimeLabel()
{
    auto milliseconds { m_saverTimer.remainingTime() };
    auto seconds { milliseconds / 1'000 };
    auto minutes { seconds / 60 };
    seconds %= 60;

    QString label = "Saving calls";
    if (minutes != 0 or seconds != 0) {
        label += " on ";
        if (minutes != 0)
            label += tr("%1 minute%2%3 ").arg(QString::number(minutes),
                                              (minutes == 1 ? "" : "s"), (seconds == 0 ? "" : ","));
        if (seconds != 0)
            label += tr("%1 second%2").arg(QString::number(seconds),
                                                 (seconds == 1 ? "." : "s."));
    } else {
        label += "...";
    }

    m_ui->remainingTimeLabel->setText(label);
    m_saverTimer1s.start();
}

void MainWindow::updateHoveredLabel(const QString& label)
{
    m_ui->currentBarSetValue->setText(label);
}
