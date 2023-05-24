#include "mainwindow.hpp"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
    , m_totalCalls(0)
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
    m_lockerTimer.setInterval(100);
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
                                    .arg(QString::number(m_totalCalls), m_totalCalls == 1 ? tr("call") : tr("calls")));

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
    if (not isDBEmpty()) {
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
    m_totalCalls = m_db.value(index).toInt();
    if (m_totalCalls == -1)
        m_totalCalls = 0;

    setLabel(true);
    m_db.close();
}

void MainWindow::error(const QString& message)
{
    QMessageBox::critical(this, PROGRAM_NAME, message);
}

bool MainWindow::isDBEmpty()
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
    msgBox.setWindowTitle(tr("About"));
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

    ++m_totalCalls;
    m_calls[m_totalCalls] = QTime::currentTime().toString("hh:mm:ss");
    m_notSavedCalls[m_totalCalls] = m_calls[m_totalCalls];
    setLabel(false, "Last registered");
    m_chart->setValue(dayOfWeek() - 1, m_totalCalls);
    m_modified = true;
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

    if (m_totalCalls == 0) {
        error(tr("Registered calls cannot be below zero."));
        return;
    }

    m_calls.remove(m_totalCalls);
    m_notSavedCalls[m_totalCalls] = m_calls[m_totalCalls];
    --m_totalCalls;
    setLabel(false, "Last removed");
    m_chart->setValue(dayOfWeek() - 1, m_totalCalls);
    m_modified = true;
}

void MainWindow::saveCalls()
{
    if (not m_modified and not sender())
        return;
    if (not m_db.open())
        return;

    QString statement;

    for (const int call : m_notSavedCalls.keys()) {
        auto time { m_notSavedCalls[call] };
        statement = QString("INSERT INTO Calls (calls, date, time, username) VALUES ('%1', '%2', '%3', '%4')")
                        .arg(QString::number(call), m_datetime, time, m_username);
        if (not m_db.exec(statement)) {
            error(tr("Couldn't execute statement."));
            error(m_db.query().lastError().text());
            m_db.close();
            return;
        }
    }

    m_notSavedCalls.clear();
    m_db.close();

    if (sender() == m_ui->saveButton) {
        QMessageBox::information(this, PROGRAM_NAME, tr("Calls have been saved."));
    }

    m_saverTimer.start();
    auto time { QDateTime::currentDateTime().toString("hh:mm:ss") };
    m_ui->lastSavedLabel->setText(tr("Last saved at %1").arg(time));
    m_modified = false;
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
