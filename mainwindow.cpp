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

    connect(&m_db, &Database::error, this, &MainWindow::error);
    connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(m_ui->newButton, &QPushButton::clicked, this, &MainWindow::newCall);
    connect(m_ui->removeButton, &QPushButton::clicked, this, &MainWindow::removeCall);
    connect(m_ui->saveButton, &QPushButton::clicked, this, &MainWindow::saveCalls);
    connect(&m_saverTimer, &QTimer::timeout, this, &MainWindow::saveCalls);
    connect(&m_saverTimer1s, &QTimer::timeout, this, &MainWindow::setRemainingTimeLabel);
    connect(&m_datetimeTimer, &QTimer::timeout, this, &MainWindow::setDateTime);
    connect(m_ui->screenshotButton, &QPushButton::clicked, this, &MainWindow::saveScreenShot);
    connect(&m_lockerTimer, &QTimer::timeout, this, &MainWindow::unlockNewButton);

    m_username = qgetenv("USER");
    if (m_username.isEmpty())
        m_username = qgetenv("USERNAME");
    m_ui->usernameLabel->setText(tr("User: ") + m_username);

    setDateTime();
    setTodaysCalls();

    m_lockerTimer.setInterval(5'000);
    m_saverTimer.setInterval(300'000); // 5 minutes.
    m_saverTimer1s.setInterval(1'000);
    m_datetimeTimer.setInterval(1'000);
    m_datetimeTimer.start();
    m_saverTimer.start();
    m_saverTimer1s.start();

    m_ui->newButton->setShortcut(Qt::CTRL | Qt::Key_A);
    m_ui->removeButton->setShortcut(Qt::CTRL | Qt::Key_R);
    m_ui->screenshotButton->setShortcut(Qt::CTRL | Qt::Key_P);
    m_ui->saveButton->setShortcut(Qt::CTRL | Qt::Key_S);
}

MainWindow::~MainWindow()
{
    delete m_ui;
}

void MainWindow::setLabel()
{
    m_ui->messageLabel->setText(tr("You've registered %1 %2 today.")
                                    .arg(QString::number(m_calls), m_calls == 1 ? tr("call") : tr("calls")));
}

void MainWindow::setDateTime()
{
    m_datetime = QDateTime::currentDateTime().toString("dd-MM-yyyy");
    auto time = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_ui->datetimeLabel->setText(tr("Date: %1 %2").arg(m_datetime, time));
}

void MainWindow::setTodaysCalls()
{
    if (not m_db.open())
        return;
    // shouldUpdate() will help us because it checks if there's at least one record in the database.
    // If there aren't any, we'll get 'not positioned on a valid record' error message, so to avoid this:
    if (not shouldUpdate()) {
        m_db.close();
        setLabel();
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

    setLabel();
    m_db.close();
}

void MainWindow::error(const QString& message)
{
    QMessageBox::critical(this, "Error", message);
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
        QMessageBox::warning(this, tr("Warning"), tr("You have to wait a couple of secs to register a new call."));
        return;
    } else {
        m_lock = true;
        m_lockerTimer.start();
    }

    ++m_calls;
    setLabel();
}

void MainWindow::removeCall()
{
    if (m_lock) {
        QMessageBox::warning(this, tr("Warning"), tr("You have to wait a couple of secs to remove a call."));
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
    setLabel();
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
        QMessageBox::information(this, tr("Information"), tr("Calls have been saved."));
    }

    m_saverTimer.start();
}

QPixmap MainWindow::takeScreenShot()
{
    QRect rect { geometry() };
    int x { rect.x() };
    int y { rect.y() };
    int width { rect.width() };
    int height { rect.height() };
    QScreen* s { screen() };
    QPixmap pixmap { s->grabWindow(0, x, y, width, height) };
    return pixmap;
}

void MainWindow::saveScreenShot()
{
    auto filename { QFileDialog::getSaveFileName(this, tr("Save screenshot"), tr("Images (*.png *.jpg *.jpeg)")) };
    if (filename.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Not saving screenshot."));
        return;
    }

    if (not filename.endsWith(".png") and not filename.endsWith(".jpg") and not filename.endsWith(".jpeg"))
        filename += ".png";

    if (QFile file(filename); file.exists()) {
        auto answer = QMessageBox::question(this, tr("Confirm"), tr("File %1 exists. Do you want to overload it?").arg(filename));
        if (answer != QMessageBox::Yes)
            return;
    }

    auto pixmap { takeScreenShot() };
    pixmap.save(filename, "PNG");
    QMessageBox::information(this, tr("Information"), tr("Screenshot saved."));
}

void MainWindow::setRemainingTimeLabel()
{
    int milliseconds { m_saverTimer.remainingTime() };
    QString minutes;

    {
        int s = milliseconds / 1'000;
        int m = s / 60;
        minutes = QString::number(m);
    }

    auto label { tr("Saving calls on %1 minute%2").arg(minutes, (minutes == "1" ? "." : "s.")) };
    m_ui->remainingTimeLabel->setText(label);
    m_saverTimer1s.start();
}
