#include "mainwindow.hpp"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
    , m_totalCalls(0)
    , m_db()
    , m_lock(false)
{
    m_ui->setupUi(this);
    m_geometry = new Geometry(this);
    m_ui->lastSavedLabel->setText("");
    m_ui->lastCallLabel->setText("");
    m_ui->currentBarSetValue->setText("");

    connect(&m_db, &Database::error, this, &MainWindow::error);
    connect(m_ui->actionAboutQt, &QAction::triggered, this, &MainWindow::about);
    connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(m_ui->newButton, &QPushButton::clicked, this, &MainWindow::newCall);
    connect(m_ui->removeButton, &QPushButton::clicked, this, &MainWindow::removeCall);
    connect(m_ui->saveButton, &QPushButton::clicked, this, &MainWindow::saveCalls);
    connect(m_ui->oldCallsButton, &QPushButton::clicked, this, &MainWindow::oldCalls);
    connect(&m_saverTimer, &QTimer::timeout, this, &MainWindow::saveCalls);
    connect(&m_saverTimer1s, &QTimer::timeout, this, &MainWindow::setRemainingTimeLabel);
    connect(&m_datetimeTimer, &QTimer::timeout, this, &MainWindow::setDateTime);
    connect(&m_lockerTimer, &QTimer::timeout, this, &MainWindow::unlockNewButton);

    m_username = qgetenv("USER");
    if (m_username.isEmpty())
        m_username = qgetenv("USERNAME");
    m_ui->usernameLabel->setText(tr("User: ") + m_username);

    m_chart = new Chart(m_db, m_username, m_ui->centralwidget);
    m_chart->setGeometry(QRect(
        m_geometry->initialChartX(), m_geometry->initialChartY(),
        m_geometry->initialChartWidth(), m_geometry->initialChartHeight()
        )
    );
    m_chart->setToolTip(tr("You've registered %1 call%2this week.").arg(QString::number(m_chart->totalCalls()),
                                                                        m_chart->totalCalls() == 1 ? tr(" ") : tr("s ")));
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

    // Shortcut depends on current locale. English and Spanish are the only ones currently supported. English is the default.
    // ToolTips for these buttons are properly set in the translations files.
    QKeyCombination newButton(Qt::CTRL | Qt::Key_A);
    QKeyCombination removeButton(Qt::CTRL | Qt::Key_R);
    QKeyCombination oldCallsButton(Qt::CTRL | Qt::Key_F);
    QKeyCombination saveButton(Qt::CTRL | Qt::Key_S);
    auto localeName(QLocale::system().name());

    if (localeName.startsWith("es")) {
        removeButton = Qt::CTRL | Qt::Key_E;
        oldCallsButton = Qt::CTRL | Qt::Key_B;
        saveButton = Qt::CTRL | Qt::Key_G;
    }

    m_ui->newButton->setShortcut(QKeySequence(newButton));
    m_ui->removeButton->setShortcut(QKeySequence(removeButton));
    m_ui->oldCallsButton->setShortcut(QKeySequence(oldCallsButton));
    m_ui->saveButton->setShortcut(QKeySequence(saveButton));
}

MainWindow::~MainWindow()
{
    delete m_ui;
    delete m_geometry;
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    // Because resizeEvent() gets called when window is first created. Let's leave Qt do its stuff first and then me.
    static bool firstTime {true};
    if (firstTime) {
        firstTime = false;
        return;
    }

    m_geometry->setNewSize(event->size());

    m_ui->datetimeLabel->setGeometry(m_geometry->datetime());
    m_ui->messageLabel->setGeometry(m_geometry->messageLabel());
    m_ui->newButton->setGeometry(m_geometry->newButton());
    m_ui->removeButton->setGeometry(m_geometry->removeButton());
    m_ui->lastSavedLabel->setGeometry(m_geometry->lastSaved());
    m_ui->remainingTimeLabel->setGeometry(m_geometry->remainingTime());
    m_ui->lastCallLabel->setGeometry(m_geometry->lastRegisteredCall());
    m_ui->currentBarSetValue->setGeometry(m_geometry->currentBarSetValue());
    m_chart->setGeometry(m_geometry->chart());
    m_ui->oldCallsButton->setGeometry(m_geometry->oldCallsButton());
    m_ui->saveButton->setGeometry(m_geometry->saveButton());
}

void MainWindow::setLabel(bool justMessageLabel, const QString& prefix = "")
{
    m_ui->messageLabel->setText(tr("You've registered %1 call%2today.")
                                    .arg(QString::number(m_totalCalls), (m_totalCalls == 1 ? " " : "s ")));

    if (not justMessageLabel) {
        auto time { QDateTime::currentDateTime().toString("hh:mm:ss") };
        m_ui->lastCallLabel->setText(tr("%1 call at %2").arg(prefix, time));
    }
}

void MainWindow::setDateTime()
{
    const auto fmt {tr("ddd, dd-MM-yyyy hh:mm:ss")}; // I want abbreviated day name in English, and full day name in Spanish.
    auto dt {QDateTime::currentDateTime()};
    m_datetime = dt.toString("dd-MM-yyyy");
    auto datetime {QLocale::system().toString(dt, fmt)};
    m_ui->datetimeLabel->setText(tr("Date: %1").arg(datetime));
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

int MainWindow::dayOfWeek() const
{
    QDate date { QDate::currentDate() };
    return date.dayOfWeek();
}

void MainWindow::about(bool checked)
{
    Q_UNUSED(checked);

    // Because this function is used both for about this program as for about Qt.
    if (sender() == m_ui->actionAboutQt) {
        QMessageBox::aboutQt(this);
        return;
    }

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

// Returns false if newCall() and/or removeCall() can proceed with their stuff.
bool MainWindow::isLocked(const QString& action)
{
    if (m_lock) {
        QMessageBox::warning(this, PROGRAM_NAME, tr("You have to wait a couple of secs to %1 a new call.").arg(action));
        return true;
    } else {
        m_lock = true;
        m_lockerTimer.start();
    }

    return false;
}

void MainWindow::newCall()
{
    if (isLocked(tr("register")))
        return;

    ++m_totalCalls;
    m_calls[m_totalCalls] = QTime::currentTime().toString("hh:mm:ss");
    m_notSavedCalls[m_totalCalls] = m_calls[m_totalCalls];
    setLabel(false, tr("Last registered"));
    m_chart->setValue(dayOfWeek() - 1, m_totalCalls, Chart::Action::Add);
    m_chart->setToolTip(tr("You've registered %1 call%2this week.").arg(QString::number(m_chart->totalCalls()),
                                                                        m_chart->totalCalls() == 1 ? tr(" ") : tr("s ")));
    m_modified = true;
}

void MainWindow::removeCall()
{
    if (isLocked("remove"))
        return;

    if (m_totalCalls == 0) {
        error(tr("Registered calls cannot be below zero."));
        return;
    }

    m_calls.remove(m_totalCalls);
    m_notSavedCalls[m_totalCalls] = m_calls[m_totalCalls];
    --m_totalCalls;
    setLabel(false, tr("Last removed"));
    m_chart->setValue(dayOfWeek() - 1, m_totalCalls, Chart::Action::Remove);
    m_chart->setToolTip(tr("You've registered %1 call%2this week.").arg(QString::number(m_chart->totalCalls()),
                                                                        m_chart->totalCalls() == 1 ? tr(" ") : tr("s ")));
    m_modified = true;
}

void MainWindow::saveCalls()
{
    if (not m_modified and not sender())
        return;
    if (not m_db.open())
        return;

    QString statement;

    for (const auto& time : qAsConst(m_notSavedCalls)) {
        int calls { m_notSavedCalls.key(time) };
        statement = QString("INSERT INTO Calls (calls, date, time, username) VALUES ('%1', '%2', '%3', '%4')")
                        .arg(QString::number(calls), m_datetime, time, m_username);
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

void MainWindow::oldCalls()
{
    CallsFinder cf(m_db);
    QEventLoop loop;
    connect(&cf, &CallsFinder::closed, &loop, &QEventLoop::quit);
    cf.show();
    loop.exec();
}

void MainWindow::setRemainingTimeLabel()
{
    auto milliseconds { m_saverTimer.remainingTime() };
    auto seconds { milliseconds / 1'000 };
    auto minutes { seconds / 60 };
    seconds %= 60;

    auto label = tr("Saving calls");
    if (minutes != 0 or seconds != 0) {
        label += tr(" on ");
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

void MainWindow::updateHoveredLabel(const QString& label, bool isToday)
{
    m_ui->currentBarSetValue->setText(label);
    auto font { m_ui->messageLabel->font() };
    font.setBold(isToday and not label.isEmpty());
    m_ui->messageLabel->setFont(font);
}

// END OF MAINWINDOW DEFINITION
// BEGINNING OF MainWindow::Geometry definition
MainWindow::Geometry::Geometry(MainWindow* mainWindow, QObject *parent)
    : QObject{parent}
    , m_mainWindow(mainWindow)
    , m_size(m_mainWindow->size())
    , m_initialWindowWidth(m_mainWindow->size().width())
    , m_initialWindowHeight(m_mainWindow->size().height())
    , m_initialDatetimeLabelX(m_mainWindow->m_ui->datetimeLabel->x())
    , m_initialMessageLabelY(m_mainWindow->m_ui->messageLabel->y())
    , m_initialLastSavedLabelY(m_mainWindow->m_ui->lastSavedLabel->y())
    , m_initialRemainingTimeLabelY(m_mainWindow->m_ui->remainingTimeLabel->y())
    , m_initialLastCallLabelX(m_mainWindow->m_ui->lastCallLabel->x())
    , m_initialLastCallLabelY(m_mainWindow->m_ui->lastCallLabel->y())
    , m_initialCurrentBarSetValueX(m_mainWindow->m_ui->currentBarSetValue->x())
    , m_initialCurrentBarSetValueY(m_mainWindow->m_ui->currentBarSetValue->y())
    , m_initialChartX(280)
    , m_initialChartY(30)
    , m_initialChartWidth(675)
    , m_initialChartHeight(381)
    , m_initialOldCallsButtonX(m_mainWindow->m_ui->oldCallsButton->x())
    , m_initialOldCallsButtonY(m_mainWindow->m_ui->oldCallsButton->y())
    , m_initialSaveButtonX(m_mainWindow->m_ui->saveButton->x())
    , m_initialSaveButtonY(m_mainWindow->m_ui->saveButton->y())
{
}

void MainWindow::Geometry::setNewSize(QSize size)
{
    m_size = size;
}

int MainWindow::Geometry::initialChartX() const
{
    return m_initialChartX;
}
int MainWindow::Geometry::initialChartY() const
{
    return m_initialChartY;
}

int MainWindow::Geometry::initialChartWidth() const
{
    return m_initialChartWidth;
}
int MainWindow::Geometry::initialChartHeight() const
{
    return m_initialChartHeight;
}

QRect MainWindow::Geometry::datetime() const
{
    return QRect(
        m_size.width() - (m_initialWindowWidth - m_initialDatetimeLabelX),
        m_mainWindow->m_ui->datetimeLabel->y(),
        m_mainWindow->m_ui->datetimeLabel->rect().width(),
        m_mainWindow->m_ui->datetimeLabel->rect().height()
        );
}

QRect MainWindow::Geometry::messageLabel() const
{
    return QRect(
        m_mainWindow->m_ui->messageLabel->x(),
        m_size.height() / 2 - 70,
        m_mainWindow->m_ui->messageLabel->rect().width(),
        m_mainWindow->m_ui->messageLabel->rect().height()
        );
}

QRect MainWindow::Geometry::newButton() const
{
    return QRect(
        m_mainWindow->m_ui->newButton->x(),
        m_size.height() / 2 - 30, // messageLabel's y + 50.
        m_mainWindow->m_ui->newButton->rect().width(),
        m_mainWindow->m_ui->newButton->rect().height()
        );
}

QRect MainWindow::Geometry::removeButton() const
{
    return QRect(
        m_mainWindow->m_ui->removeButton->x(),
        m_size.height() / 2 - 30, // messageLabel's y + 50.
        m_mainWindow->m_ui->removeButton->rect().width(),
        m_mainWindow->m_ui->removeButton->rect().height()
        );
}

QRect MainWindow::Geometry::lastSaved() const
{
    return QRect(
        m_mainWindow->m_ui->lastSavedLabel->x(),
        m_size.height() - (m_initialWindowHeight - m_initialLastSavedLabelY),
        m_mainWindow->m_ui->lastSavedLabel->rect().width(),
        m_mainWindow->m_ui->lastSavedLabel->rect().height()
        );
}

QRect MainWindow::Geometry::remainingTime() const
{
    return QRect(
        m_mainWindow->m_ui->remainingTimeLabel->x(),
        m_size.height() - (m_initialWindowHeight - m_initialRemainingTimeLabelY),
        m_mainWindow->m_ui->remainingTimeLabel->rect().width(),
        m_mainWindow->m_ui->remainingTimeLabel->rect().height()
        );
}

QRect MainWindow::Geometry::lastRegisteredCall() const
{
    return QRect(
        m_size.width() - (m_initialWindowWidth - m_initialLastCallLabelX),
        m_size.height() - (m_initialWindowHeight - m_initialLastCallLabelY),
        m_mainWindow->m_ui->lastCallLabel->rect().width(),
        m_mainWindow->m_ui->lastCallLabel->rect().height()
        );
}

QRect MainWindow::Geometry::currentBarSetValue() const
{
    return QRect(
        m_size.width() - (m_initialWindowWidth - m_initialCurrentBarSetValueX),
        m_size.height() - (m_initialWindowHeight - m_initialCurrentBarSetValueY),
        m_mainWindow->m_ui->currentBarSetValue->rect().width(),
        m_mainWindow->m_ui->currentBarSetValue->rect().height()
        );
}

QRect MainWindow::Geometry::chart() const
{
    return QRect(
        m_initialChartX,
        m_initialChartY,
        m_size.width() - (m_initialWindowWidth - m_initialChartWidth),
        m_size.height() - (m_initialWindowHeight - m_initialChartHeight)
        );
}

QRect MainWindow::Geometry::oldCallsButton() const
{
    return QRect(
        m_size.width() - (m_initialWindowWidth - m_initialOldCallsButtonX),
        m_size.height() - (m_initialWindowHeight - m_initialOldCallsButtonY),
        m_mainWindow->m_ui->oldCallsButton->rect().width(),
        m_mainWindow->m_ui->oldCallsButton->rect().height()
        );
}

QRect MainWindow::Geometry::saveButton() const
{
    return QRect(
        m_size.width() - (m_initialWindowWidth - m_initialSaveButtonX),
        m_size.height() - (m_initialWindowHeight - m_initialSaveButtonY),
        m_mainWindow->m_ui->saveButton->rect().width(),
        m_mainWindow->m_ui->saveButton->rect().height()
        );
}
