#ifndef DATABASE_H
#define DATABASE_H

#include <QDir>
#include <QStandardPaths>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QObject>

class Database : public QObject
{
    Q_OBJECT
    QSqlDatabase m_db;
    QSqlQuery m_query;

    void setDBEnvironmentUp();
    void createDB();
public:
    explicit Database(QObject *parent = nullptr);
    ~Database();
    bool open();
    void close();
    bool exec(const QString& statement);
    QSqlQuery& query();
    QSqlRecord record();
    QVariant value(int index) const;
signals:
    void error(QString message);
};

#endif // DATABASE_H
