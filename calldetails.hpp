#ifndef CALLDETAILS_HPP
#define CALLDETAILS_HPP

#include <QCloseEvent>
#include <QIcon>
#include <QMessageBox>
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

    void prepareTableWidget();
    void setCalls();
public:
    CallDetails(Database& db, const QString& datetime, QWidget* parent = nullptr);
    ~CallDetails();
protected:
    void closeEvent(QCloseEvent* event);
signals:
    void closed();
};

#endif // CALLDETAILS_HPP
