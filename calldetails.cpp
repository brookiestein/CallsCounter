#include "calldetails.hpp"
#include "ui_calldetails.h"

CallDetails::CallDetails(Database& db, const QString& datetime, QWidget* parent) :
    QWidget(parent)
    , m_ui(new Ui::CallDetails)
    , m_db(db)
    , m_datetime(datetime)
{
    m_ui->setupUi(this);
    m_tw = m_ui->tableWidget;

    setWindowIcon(QIcon("assets/icon.ico"));
    prepareTableWidget();
    setCalls();
}

CallDetails::~CallDetails()
{
    delete m_ui;
}

void CallDetails::closeEvent(QCloseEvent* event)
{
    emit closed();
    QWidget::closeEvent(event);
}

void CallDetails::prepareTableWidget()
{
    QStringList headers;
    headers << "Calls" << "Date" << "Time";

    m_tw->setColumnCount(headers.size());
    m_tw->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tw->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_tw->setEditTriggers(QTableWidget::NoEditTriggers);
    m_tw->setHorizontalHeaderLabels(headers);
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

    while (query.next()) {
        auto calls { query.value(callsID).toString() };
        auto time { query.value(timeID).toString() };
        auto rowCount { m_tw->rowCount() };

        m_tw->insertRow(rowCount);
        m_tw->setItem(rowCount, 0, new QTableWidgetItem(calls));
        m_tw->setItem(rowCount, 1, new QTableWidgetItem(m_datetime));
        m_tw->setItem(rowCount, 2, new QTableWidgetItem(time));
    }

    m_db.close();
}
