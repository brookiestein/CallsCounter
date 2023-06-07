#ifndef CALLSFINDER_HPP
#define CALLSFINDER_HPP

#include <QCloseEvent>
#include <QMessageBox>
#include <QRect>
#include <QResizeEvent>
#include <QSize>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QWidget>

#include "database.hpp"

namespace Ui { class CallsFinder; }

class CallsFinder : public QWidget
{
    Q_OBJECT
    Ui::CallsFinder* m_ui;
    QTableWidget* m_tw;
    Database& m_db;
    class Geometry;
    Geometry* m_geometry;

    void prepareTableWidget();
public:
    CallsFinder(Database& db, QWidget* parent = nullptr);
    ~CallsFinder();
protected:
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
signals:
    void closed();
private slots:
    void searchCalls();
};

class CallsFinder::Geometry : public QObject
{
    Q_OBJECT
    CallsFinder* m_callsFinder;
    QSize m_size;
    const int m_initialWindowWidth;
    const int m_initialWindowHeight;
    const int m_initialFromLabelX;
    const int m_initialFromEditX;
    const int m_initialToLabelX;
    const int m_initialToEditX;
    const int m_initialSearchButtonX;
    const int m_initialTableWidgetX;
    const int m_initialTableWidgetY;
    const int m_initialTableWidgetWidth;
    const int m_initialTableWidgetHeight;
public:
    Geometry(CallsFinder* callsFinder, QObject* parent = nullptr);
    void setSize(QSize size);
    QRect fromLabel() const;
    QRect fromDateEdit() const;
    QRect toLabel() const;
    QRect toDateEdit() const;
    QRect searchButton() const;
    QRect tableWidget() const;
};

#endif // CALLSFINDER_HPP
