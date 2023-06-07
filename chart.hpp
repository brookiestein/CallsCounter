#ifndef CHART_HPP
#define CHART_HPP

#include <QBarCategoryAxis>
#include <QBarSeries>
#include <QBarSet>
#include <QChart>
#include <QChartView>
#include <QDate>
#include <QEventLoop>
#include <QIcon>
#include <QMap>
#include <QPainter>
#include <QStringList>
#include <QSqlRecord>
#include <QValueAxis>
#include <QWidget>

#include "calldetails.hpp"
#include "database.hpp"

namespace Ui { class Chart; }

class Chart : public QChartView
{
    Q_OBJECT
    Ui::Chart *m_ui;
    QString m_username;
    QChart m_chart;
    QBarCategoryAxis m_x_axis;
    QValueAxis m_y_axis;
    QBarSeries m_series;
    QBarSet m_set;
    QStringList m_categories;
    Database& m_db;
    QMap<quint8, QString> m_dates;
    int m_totalCalls;

    void setValues();
public:
    Chart(Database& db, const QString& username, QWidget* parent = nullptr);
    ~Chart();
    enum class Action { Add, Remove };
    void setValue(int day, int value, Action action);
    int totalCalls() const;
signals:
    void error(const QString& message);
    void updateHoveredLabel(const QString& label, bool isToday);
private slots:
    void hovered(bool status, int index);
    void clicked(int index);
};

#endif // CHART_HPP
