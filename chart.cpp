#include "chart.hpp"
#include "ui_chart.h"

Chart::Chart(Database& db, const QString& username, QWidget* parent) :
    QChartView(parent)
    , m_ui(new Ui::Chart)
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
    connect(&m_set, &QBarSet::clicked, this, &Chart::clicked);
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
    int dayOfWeek { currentDate.dayOfWeek() };
    // We're going to iterate until daysToSubstract gets to -1.
    // We want to start adding calls from Monday (0) to today.
    int daysToSubstract { dayOfWeek - 1 };

    for (int i {}; i < 6; ++i)
        m_set << 1;
    QMap<int, int> calls;

    while (daysToSubstract != -1) {
        auto date { currentDate.addDays(-daysToSubstract).toString("dd-MM-yyyy") };
        auto statement = QString("SELECT MAX(calls) AS calls FROM Calls WHERE date = '%1'").arg(date);
        if (not m_db.exec(statement)) {
            if (daysToSubstract == 0) {
                break;
            } else {
                --daysToSubstract;
                continue;
            }
        }

        auto& query { m_db.query() };
        auto record { query.record() };
        if (not query.first() and daysToSubstract == 0)
            break;

        int callsID { record.indexOf("calls") };
        auto ncalls { query.value(callsID).toInt() };

        int index { dayOfWeek - daysToSubstract-- };
        calls[index - 1] = ncalls;
        m_set.replace(index - 1, ncalls);
        m_dates[index - 1] = date;
    }

    for (int i {}; i < m_set.count(); ++i) {
        if (m_set[i] == 1 and calls.key(calls[i]) == 0) {
            m_set.replace(i, 0);
        }
    }

    m_db.close();
}

void Chart::setValue(int day, int value)
{
    m_set.replace(day, value);
    update();
}

void Chart::hovered(bool status, int index)
{
    if (not status) {
        emit updateHoveredLabel("", false);
        return;
    }

    auto calls { QString::number(m_set[index]) };
    bool isToday { (index + 1) == QDate::currentDate().dayOfWeek() };
    // FIXME: Make this string apropriate to be translated.
    auto label = tr("%1 registered call(s) today.").arg(calls);
    if (not isToday) {
        label.replace(tr("today."), tr("on %1.").arg(m_categories[index]));
    }
    emit updateHoveredLabel(label, isToday);
}

void Chart::clicked(int index)
{
    auto datetime { m_dates[index] };
    bool isToday { (index + 1) == QDate::currentDate().dayOfWeek() };
    QString dayname { isToday ? tr("Today") : m_categories[index].toStdString().c_str() };
    CallDetails cd(m_db, datetime, dayname);
    QEventLoop loop;
    connect(&cd, &CallDetails::closed, &loop, &QEventLoop::quit);
    cd.show();
    loop.exec();
}
