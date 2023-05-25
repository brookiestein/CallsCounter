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

void CallDetails::resizeEvent(QResizeEvent* event)
{
    static bool firstTime {true};
    if (firstTime) {
        firstTime = false;
        return;
    }

    // Difference between window size and tableWidget size.
    const int widthDiff {40};
    const int heightDiff {70};
    // No need to modify label width and height.
    const int labelWidth {m_ui->label->rect().width()};
    const int labelHeight {m_ui->label->rect().height()};

    auto newSize {event->size()};
    const int labelx {newSize.width() / 2 - labelWidth / 2};
    auto x {20};
    auto y {50};
    auto width {newSize.width() - widthDiff};
    auto height {newSize.height() - heightDiff};

    m_tw->setGeometry(QRect(x, y, width, height));
    m_ui->label->setGeometry(QRect(labelx, 20, labelWidth, labelHeight));
}

void CallDetails::prepareTableWidget()
{
    QStringList headers;
    headers << "Calls" << "Date" << "Time";

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
