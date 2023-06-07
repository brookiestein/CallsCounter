#ifndef CALLDETAILS_HPP
#define CALLDETAILS_HPP

#include <QCloseEvent>
#include <QDate>
#include <QIcon>
#include <QKeySequence>
#include <QLocale>
#include <QMessageBox>
#include <QRect>
#include <QResizeEvent>
#include <QSize>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QWidget>

#include "database.hpp"

namespace Ui { class CallDetails; }

class CallDetails : public QWidget
{
    Q_OBJECT
    Ui::CallDetails *m_ui;
    QTableWidget* m_tw;
    Database& m_db;
    QString m_datetime;
    QString m_dayname;
    class Geometry;
    Geometry* m_geometry;

    void prepareTableWidget();
public:
    CallDetails(Database& db, const QString& datetime, const QString& dayname, QWidget* parent = nullptr);
    ~CallDetails();
protected:
    void closeEvent(QCloseEvent* event);
    void resizeEvent(QResizeEvent* event);
signals:
    void closed();
private slots:
    void setCalls();
};

class CallDetails::Geometry : public QObject
{
    Q_OBJECT
    CallDetails* m_callDetails;
    QSize m_size;
    const int m_initialWindowWidth;
    const int m_initialWindowHeight;
    const int m_initialLabelX;
    const int m_initialLabelY;
    const int m_initialLabelWidth;
    const int m_initialLabelHeight;
    const int m_initialUpdateButtonX;
    const int m_initialTableWidgetX;
    const int m_initialTableWidgetY;
    const int m_initialTableWidgetWidth;
    const int m_initialTableWidgetHeight;
public:
    Geometry(CallDetails* callDetails);
    void setSize(QSize size);
    QRect label() const;
    QRect updateButton() const;
    QRect tableWidget() const;
};

#endif // CALLDETAILS_HPP
