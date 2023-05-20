#include "chart.hpp"
#include "ui_chart.h"

Chart::Chart(Database& db, const QString& username, QWidget* parent) :
    QChartView(parent),
    m_ui(new Ui::Chart)
    , m_db(db)
    , m_username(username)
    , m_series(QBarSeries())
    , m_set(QBarSet(m_username))
    , m_chart(QChart())
    , m_x_axis(QBarCategoryAxis())
    , m_y_axis(QValueAxis())
{
    m_ui->setupUi(this);
    m_categories << tr("Monday") << tr("Tuesday") << tr("Wednesday") << tr("Thursday")
                 << tr("Friday") << tr("Saturday");

    setValues();

    m_series.append(&m_set);

    m_chart.addSeries(&m_series);
    m_chart.setTitle(tr("%1's Weekly Performance").arg(m_username));
    m_chart.setAnimationOptions(QChart::SeriesAnimations);

    m_x_axis.append(m_categories);
    m_chart.addAxis(&m_x_axis, Qt::AlignBottom);
    m_series.attachAxis(&m_x_axis);

    m_y_axis.setRange(0, 200);
    m_chart.addAxis(&m_y_axis, Qt::AlignLeft);
    m_series.attachAxis(&m_y_axis);

    m_chart.legend()->hide();
    setChart(&m_chart);

    setRenderHint(QPainter::Antialiasing);

    connect(&m_set, &QBarSet::hovered, this, &Chart::hovered);
}

Chart::~Chart()
{
    delete m_ui;
}

void Chart::setValues()
{
    if (not m_db.open()) {
        emit error(tr("[%1]: Database couldn't be opened.").arg(Q_FUNC_INFO));
        return;
    }

    QDate currentDate { QDate::currentDate() };
    auto today { currentDate.toString("dd-MM-yyyy") };
    auto oneWeekBefore { currentDate.addDays(-7).toString("dd-MM-yyyy") };
    auto statement = QString("SELECT calls, date FROM Calls WHERE date BETWEEN '%1' AND '%2'").arg(oneWeekBefore, today);
    if (not m_db.exec(statement)) {
        m_db.close();
        emit error(tr("[%1]: Couldn't execute statement.").arg(Q_FUNC_INFO));
        return;
    }

    auto& query { m_db.query() };
    auto record { query.record() };
    int callsID { record.indexOf("calls") };
    int dateID { record.indexOf("date") };

    //: If we're starting the week, don't show any other day.
    if (query.first()) {
        int day { currentDate.dayOfWeek() };
        if (day == 1) {
            int calls {};
            int dbDay { QDate::fromString(query.value(dateID).toString()).dayOfWeek() };
            do {
                calls = query.value(callsID).toInt();
            } while (day != dbDay);
            m_set << calls;
            m_db.close();
            return;
        }
    } else { // Nothing else to do.
        return;
    }

    for (int i {}; i < 6; ++i)
        m_set << 1;

    query.previous();
    while (query.next()) {
        auto dateStr { query.value(dateID).toString() };
        auto date { QDate::fromString(dateStr, "dd-MM-yyyy") };
        int index { date.dayOfWeek() };

        int calls { query.value(callsID).toInt() };
        qDebug() << tr("Setting barset #%1 to %2.").arg(QString::number(index - 1), QString::number(calls));
        //: If we're in the same day of week, but it isn't actually today, don't show that set until we setValue().
        if (date.dayOfWeek() == currentDate.dayOfWeek() and date != QDate::currentDate())
            calls = 0;
        m_set.replace(index - 1, calls);
    }

    for (int i {}; i < m_set.count(); ++i)
        if (m_set[i] == 1)
            m_set.replace(i, 0);

    m_db.close();
}

void Chart::setValue(int day, int value)
{
    m_set.replace(day, value);
    repaint();
}

void Chart::hovered(bool status, int index)
{
    if (not status) {
        emit updateHoveredLabel("");
        return;
    }

    auto calls { QString::number(m_set[index]) };
    QString dayName;
    switch (index) {
    case 0:
        dayName = "Monday";
        break;
    case 1:
        dayName = "Tuesday";
        break;
    case 2:
        dayName = "Wednesday";
        break;
    case 3:
        dayName = "Thursday";
        break;
    case 4:
        dayName = "Friday";
        break;
    case 5:
        dayName = "Saturday";
    }

    emit updateHoveredLabel(tr("%1 registered calls on %2.").arg(calls, dayName));
}
