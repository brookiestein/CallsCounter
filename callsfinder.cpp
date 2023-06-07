#include "callsfinder.hpp"
#include "ui_callsfinder.h"

CallsFinder::CallsFinder(Database& db, QWidget *parent) :
    QWidget(parent)
    , m_db(db)
    , m_ui(new Ui::CallsFinder)
{
    m_ui->setupUi(this);
    m_ui->fromDateEdit->setDate(QDate::currentDate());
    m_ui->fromDateEdit->setDisplayFormat("dd/MM/yyyy");
    m_ui->toDateEdit->setDate(QDate::currentDate());
    m_ui->toDateEdit->setDisplayFormat("dd/MM/yyyy");
    m_tw = m_ui->tableWidget;
    m_geometry = new Geometry(this);

    prepareTableWidget();

    connect(m_ui->searchButton, &QPushButton::clicked, this, &CallsFinder::searchCalls);
}

CallsFinder::~CallsFinder()
{
    delete m_ui;
    delete m_geometry;
}

void CallsFinder::closeEvent(QCloseEvent* event)
{
    emit closed();
    QWidget::closeEvent(event);
}

void CallsFinder::resizeEvent(QResizeEvent* event)
{
    static bool firstTime {true};
    if (firstTime) {
        firstTime = false;
        return;
    }

    m_geometry->setSize(event->size());

    m_ui->fromLabel->setGeometry(m_geometry->fromLabel());
    m_ui->fromDateEdit->setGeometry(m_geometry->fromDateEdit());
    m_ui->toLabel->setGeometry(m_geometry->toLabel());
    m_ui->toDateEdit->setGeometry(m_geometry->toDateEdit());
    m_ui->searchButton->setGeometry(m_geometry->searchButton());
    m_ui->tableWidget->setGeometry(m_geometry->tableWidget());
}

void CallsFinder::prepareTableWidget()
{
    QStringList headers;
    headers << tr("Calls") << tr("Date") << tr("Time");

    m_tw->setColumnCount(headers.size());
    m_tw->setSelectionMode(QAbstractItemView::NoSelection);
    m_tw->setEditTriggers(QTableWidget::NoEditTriggers);
    m_tw->setHorizontalHeaderLabels(headers);
    m_tw->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void CallsFinder::searchCalls()
{
    if (not m_db.open()) {
        QMessageBox::critical(this, PROGRAM_NAME, tr("[%1]: Database couldn't be open.").arg(Q_FUNC_INFO));
        return;
    }

    int rowCount { m_tw->rowCount() };
    if (rowCount > 0) {
        for (int i {}; i < rowCount; ++i)
            m_tw->removeRow(0);
    }
    rowCount = m_tw->rowCount();

    auto from { m_ui->fromDateEdit->date().toString("dd-MM-yyyy") };
    auto to { m_ui->toDateEdit->date().toString("dd-MM-yyyy") };
    auto statement { QString("SELECT calls, date, time FROM Calls WHERE date BETWEEN '%1' AND '%2'").arg(from, to) };
    if (not m_db.exec(statement)) {
        m_db.close();
        QMessageBox::critical(this, PROGRAM_NAME, tr("[%1]: Couldn't execute statement.").arg(Q_FUNC_INFO));
        return;
    }

    auto& query { m_db.query() };
    if (not query.first()) {
        m_db.close();
        QMessageBox::warning(this, PROGRAM_NAME, tr("There aren't any registered calls between %1 and %2.").arg(from, to));
        return;
    }

    auto record { query.record() };
    int callsID { record.indexOf("calls") };
    int dateID { record.indexOf("date") };
    int timeID { record.indexOf("time") };

    query.previous();
    while (query.next()) {
        auto calls { query.value(callsID).toString() };
        auto date { query.value(dateID).toString() };
        auto time { query.value(timeID).toString() };

        m_tw->insertRow(rowCount);
        m_tw->setItem(rowCount, 0, new QTableWidgetItem(calls));
        m_tw->setItem(rowCount, 1, new QTableWidgetItem(date));
        m_tw->setItem(rowCount, 2, new QTableWidgetItem(time));
    }

    m_db.close();
}

// END OF CALLSFINDER CLASS
// BEGINNING OF GEOMETRY CLASS
CallsFinder::Geometry::Geometry(CallsFinder* callsFinder, QObject* parent)
    : QObject(parent)
    , m_callsFinder(callsFinder)
    , m_initialWindowWidth(m_callsFinder->size().width())
    , m_initialWindowHeight(m_callsFinder->size().height())
    , m_initialFromLabelX(m_callsFinder->m_ui->fromLabel->x())
    , m_initialFromEditX(m_callsFinder->m_ui->fromDateEdit->x())
    , m_initialToLabelX(m_callsFinder->m_ui->toLabel->x())
    , m_initialToEditX(m_callsFinder->m_ui->toDateEdit->x())
    , m_initialSearchButtonX(m_callsFinder->m_ui->searchButton->x())
    , m_initialTableWidgetX(m_callsFinder->m_ui->tableWidget->x())
    , m_initialTableWidgetY(m_callsFinder->m_ui->tableWidget->y())
    , m_initialTableWidgetWidth(m_callsFinder->m_ui->tableWidget->rect().width())
    , m_initialTableWidgetHeight(m_callsFinder->m_ui->tableWidget->rect().height())
{
}

void CallsFinder::Geometry::setSize(QSize size)
{
    m_size = size;
}

QRect CallsFinder::Geometry::fromLabel() const
{
    return QRect(
        m_size.width() - (m_initialWindowWidth - m_initialFromLabelX),
        m_callsFinder->m_ui->fromLabel->y(),
        m_callsFinder->m_ui->fromLabel->rect().width(),
        m_callsFinder->m_ui->fromLabel->rect().height()
        );
}

QRect CallsFinder::Geometry::fromDateEdit() const
{
    return QRect(
        m_size.width() - (m_initialWindowWidth - m_initialFromEditX),
        m_callsFinder->m_ui->fromDateEdit->y(),
        m_callsFinder->m_ui->fromDateEdit->rect().width(),
        m_callsFinder->m_ui->fromDateEdit->rect().height()
        );
}

QRect CallsFinder::Geometry::toLabel() const
{
    return QRect(
        m_size.width() - (m_initialWindowWidth - m_initialToLabelX),
        m_callsFinder->m_ui->toLabel->y(),
        m_callsFinder->m_ui->toLabel->rect().width(),
        m_callsFinder->m_ui->toLabel->rect().height()
        );
}

QRect CallsFinder::Geometry::toDateEdit() const
{
    return QRect(
        m_size.width() - (m_initialWindowWidth - m_initialToEditX),
        m_callsFinder->m_ui->toDateEdit->y(),
        m_callsFinder->m_ui->toDateEdit->rect().width(),
        m_callsFinder->m_ui->toDateEdit->rect().height()
        );
}

QRect CallsFinder::Geometry::searchButton() const
{
    return QRect(
        m_size.width() - (m_initialWindowWidth - m_initialSearchButtonX),
        m_callsFinder->m_ui->searchButton->y(),
        m_callsFinder->m_ui->searchButton->rect().width(),
        m_callsFinder->m_ui->searchButton->rect().height()
        );
}

QRect CallsFinder::Geometry::tableWidget() const
{
    return QRect(
        m_initialTableWidgetX,
        m_initialTableWidgetY,
        m_size.width() - (m_initialWindowWidth - m_initialTableWidgetWidth),
        m_size.height() - (m_initialWindowHeight - m_initialTableWidgetHeight)
        );
}
