#include "mainwindow.hpp"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
    , m_calls(0)
    , m_db()
{
    m_ui->setupUi(this);

    connect(&m_db, &Database::error, this, &MainWindow::error);
    connect(m_ui->newButton, &QPushButton::clicked, this, &MainWindow::newCall);
    connect(m_ui->saveButton, &QPushButton::clicked, this, &MainWindow::saveCalls);
    connect(&m_timer, &QTimer::timeout, this, &MainWindow::setDateTime);

    m_username = qgetenv("USER");
    if (m_username.isEmpty())
        m_username = qgetenv("USERNAME");
    m_ui->usernameLabel->setText("Usuario: " + m_username);

    setDateTime();
    setTodaysCalls();

    m_timer.setInterval(1'000);
    m_timer.start();
}

MainWindow::~MainWindow()
{
    delete m_ui;
}

void MainWindow::setLabel()
{
    m_ui->messageLabel->setText(QString("Hoy has registrado %1 %2.")
                              .arg(QString::number(m_calls), m_calls == 1 ? "llamada" : "llamadas"));
}

void MainWindow::setDateTime()
{
    m_datetime = QDateTime::currentDateTime().toString("dd-MM-yyyy");
    m_ui->datetimeLabel->setText(QString("Fecha: %1").arg(m_datetime));
}

void MainWindow::setTodaysCalls()
{
    if (not m_db.open())
        return;
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

void MainWindow::newCall()
{
    ++m_calls;
    setLabel();
}

void MainWindow::saveCalls()
{
    if (not m_db.open())
        return;

    bool update = shouldUpdate();
    QString statement;

    if (update) {
        // I prefer to use IDs in the WHERE clause, but we have no problem since datetime cannot be duplicated.
        statement = QString("UPDATE Calls SET calls='%1' WHERE username='%2' AND date='%3'")
                        .arg(QString::number(m_calls), m_username, m_datetime);
    } else {
        statement = QString("INSERT INTO Calls (calls, date, username) VALUES ('%1', '%2', '%3')")
                        .arg(QString::number(m_calls), m_datetime, m_username);
    }

    if (not m_db.exec(statement)) {
        m_db.close();
        return;
    }

    m_db.close();

    QMessageBox::information(this, "Information", "Calls have been saved.");
}
