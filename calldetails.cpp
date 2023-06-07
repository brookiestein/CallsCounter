#include "calldetails.hpp"
#include "ui_calldetails.h"

CallDetails::CallDetails(Database& db, const QString& datetime, const QString& dayname, QWidget* parent) :
    QWidget(parent)
    , m_ui(new Ui::CallDetails)
    , m_db(db)
    , m_datetime(datetime)
    , m_dayname(dayname)
{
    m_ui->setupUi(this);
    m_geometry = new Geometry(this);
    this->setWindowTitle(tr("%1's Registered Calls").arg(dayname));
    m_ui->label->setText(this->windowTitle());
    m_tw = m_ui->tableWidget;

    prepareTableWidget();
    setCalls();

    if (QDate::fromString(datetime, "dd-MM-yyyy") == QDate::currentDate()) {
        connect(m_ui->updateButton, &QPushButton::clicked, this, &CallDetails::setCalls);
        // Shortcut depends on current locale. English and Spanish are the only ones currently supported.
        QKeySequence keySequence(QLocale::system().name().startsWith("en") ? (Qt::CTRL | Qt::Key_U) : (Qt::CTRL | Qt::Key_A));
        m_ui->updateButton->setShortcut(keySequence);
    } else {
        // It has no sense to ask the DB for up to date information if it's not today,
        // because other days will never get updated.
        m_ui->updateButton->hide();
    }
}

CallDetails::~CallDetails()
{
    delete m_ui;
    delete m_geometry;
}

void CallDetails::closeEvent(QCloseEvent* event)
{
    emit closed();
    QWidget::closeEvent(event);
}

void CallDetails::resizeEvent(QResizeEvent* event)
{
    // Because resizeEvent() gets called when window is first created. Let's leave Qt do its stuff first and then me.
    static bool firstTime {true};
    if (firstTime) {
        firstTime = false;
        return;
    }

    m_geometry->setSize(event->size());
    m_ui->label->setGeometry(m_geometry->label());
    if (not m_ui->updateButton->isHidden())
        m_ui->updateButton->setGeometry(m_geometry->updateButton());
    m_tw->setGeometry(m_geometry->tableWidget());
}

void CallDetails::prepareTableWidget()
{
    QStringList headers;
    headers << tr("Calls") << tr("Date") << tr("Time");

    m_tw->setColumnCount(headers.size());
    m_tw->setSelectionMode(QAbstractItemView::NoSelection);
    m_tw->setEditTriggers(QTableWidget::NoEditTriggers);
    m_tw->setHorizontalHeaderLabels(headers);
    m_tw->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void CallDetails::setCalls()
{
    if (not m_db.open()) {
        QMessageBox::critical(this, PROGRAM_NAME, tr("Database couldn't be opened."));
        return;
    }

    auto statement = QString("SELECT calls, time FROM Calls WHERE date='%1'").arg(m_datetime);
    if (not m_db.exec(statement)) {
        QMessageBox::critical(this, PROGRAM_NAME, tr("Couldn't execute statement."));
        QMessageBox::critical(this, PROGRAM_NAME, m_db.query().lastError().text());
        m_db.close();
        return;
    }

    auto& query { m_db.query() };
    auto record { query.record() };
    int callsID { record.indexOf("calls") };
    int timeID { record.indexOf("time") };

    int rowCount { m_tw->rowCount() };
    if (rowCount > 1) {
        for (int i {}; i < rowCount; ++i)
            m_tw->removeRow(0);
    }

    while (query.next()) {
        auto calls { query.value(callsID).toString() };
        auto time { query.value(timeID).toString() };
        rowCount = m_tw->rowCount();

        m_tw->insertRow(rowCount);
        m_tw->setItem(rowCount, 0, new QTableWidgetItem(calls));
        m_tw->setItem(rowCount, 1, new QTableWidgetItem(m_datetime));
        m_tw->setItem(rowCount, 2, new QTableWidgetItem(time));
    }

    m_db.close();
}

CallDetails::Geometry::Geometry(CallDetails* callDetails) : m_callDetails(callDetails)
    , m_initialWindowWidth(m_callDetails->size().width())
    , m_initialWindowHeight(m_callDetails->size().height())
    , m_initialLabelX(m_callDetails->m_ui->label->x())
    , m_initialLabelY(m_callDetails->m_ui->label->y())
    , m_initialLabelWidth(m_callDetails->m_ui->label->rect().width())
    , m_initialLabelHeight(m_callDetails->m_ui->label->rect().height())
    , m_initialUpdateButtonX(m_callDetails->m_ui->updateButton->x())
    , m_initialTableWidgetX(m_callDetails->m_ui->tableWidget->x())
    , m_initialTableWidgetY(m_callDetails->m_ui->tableWidget->y())
    , m_initialTableWidgetWidth(m_callDetails->m_ui->tableWidget->rect().width())
    , m_initialTableWidgetHeight(m_callDetails->m_ui->tableWidget->rect().height())
{
    m_size = QSize(m_initialWindowWidth, m_initialWindowHeight);
}

void CallDetails::Geometry::setSize(QSize size)
{
    m_size = size;
}

QRect CallDetails::Geometry::label() const
{
    return QRect(
        m_size.width() / 2 - 120,
        m_initialLabelY,
        m_initialLabelWidth,
        m_initialLabelHeight
        );
}

QRect CallDetails::Geometry::updateButton() const
{
    int y { m_callDetails->m_ui->updateButton->y() };
    int width { m_callDetails->m_ui->updateButton->rect().width() };
    int height { m_callDetails->m_ui->updateButton->rect().height() };
    return QRect(
        m_size.width() - (m_initialWindowWidth - m_initialUpdateButtonX),
        y,
        width,
        height
        );
}

QRect CallDetails::Geometry::tableWidget() const
{
    return QRect(
        m_initialTableWidgetX,
        m_initialTableWidgetY,
        m_size.width() - (m_initialWindowWidth - m_initialTableWidgetWidth),
        m_size.height() - (m_initialWindowHeight - m_initialTableWidgetHeight)
        );
}
