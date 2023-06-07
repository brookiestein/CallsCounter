#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QKeyCombination>
#include <QKeySequence>
#include <QLocale>
#include <QMainWindow>
#include <QMap>
#include <QMessageBox>
#include <QPixmap>
#include <QRect>
#include <QResizeEvent>
#include <QScreen>
#include <QStandardPaths>
#include <QSqlRecord>
#include <QTime>
#include <QTimer>

#include "callsfinder.hpp"
#include "chart.hpp"
#include "database.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

namespace PerformanceMeasurer
{
class MainWindow;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
    Ui::MainWindow *m_ui;
    QMap<int, QString> m_calls;
    QMap<int, QString> m_notSavedCalls;
    bool m_modified;
    int m_totalCalls;
    QString m_datetime;
    QString m_username;
    Chart* m_chart;
    QTimer m_lockerTimer;
    QTimer m_saverTimer;
    QTimer m_saverTimer1s;
    QTimer m_datetimeTimer;
    bool m_lock;
    Database m_db;
    class Geometry;
    Geometry* m_geometry;

    void setLabel(bool justMessageLabel, const QString& prefix);
    void setDateTime();
    void setTodaysCalls();
    bool isDBEmpty();
    int dayOfWeek() const;
    bool isLocked(const QString& action);
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
protected:
    void resizeEvent(QResizeEvent* event) override;
private slots:
    void about(bool checked);
    void unlockNewButton();
    void error(const QString& message);
    void newCall();
    void removeCall();
    void saveCalls();
    void oldCalls();
    void setRemainingTimeLabel();
    void updateHoveredLabel(const QString& label, bool isToday);
};

// This class's only purpose is to calculate main window's widgets' geometry
// when resizeEvent is invoked.
class MainWindow::Geometry : public QObject
{
    Q_OBJECT
    MainWindow* m_mainWindow;
    QSize m_size;
    const int m_initialWindowWidth;
    const int m_initialWindowHeight;
    const int m_initialDatetimeLabelX;
    const int m_initialMessageLabelY;
    const int m_initialLastSavedLabelY;
    const int m_initialRemainingTimeLabelY;
    const int m_initialLastCallLabelX;
    const int m_initialLastCallLabelY;
    const int m_initialCurrentBarSetValueX;
    const int m_initialCurrentBarSetValueY;
    const int m_initialChartX;
    const int m_initialChartY;
    const int m_initialChartWidth;
    const int m_initialChartHeight;
    const int m_initialOldCallsButtonX;
    const int m_initialOldCallsButtonY;
    const int m_initialSaveButtonX;
    const int m_initialSaveButtonY;
public:
    Geometry(MainWindow* mainWindow, QObject *parent = nullptr);
    void setNewSize(QSize size);
    int initialChartX() const;
    int initialChartY() const;
    int initialChartWidth() const;
    int initialChartHeight() const;
    QRect datetime() const;
    QRect messageLabel() const;
    QRect newButton() const;
    QRect removeButton() const;
    QRect lastSaved() const;
    QRect remainingTime() const;
    QRect lastRegisteredCall() const;
    QRect currentBarSetValue() const;
    QRect chart() const;
    QRect oldCallsButton() const;
    QRect saveButton() const;
};

#endif // MAINWINDOW_H
