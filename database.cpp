#include "database.hpp"

Database::Database(QObject *parent)
    : QObject{parent}
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_query = QSqlQuery(m_db);

    setDBEnvironmentUp();
    createDB();
}

Database::~Database()
{
    if (m_db.isOpen())
        m_db.close();
}

void Database::setDBEnvironmentUp()
{
    auto actualDir { QStandardPaths::displayName(QStandardPaths::StandardLocation::DocumentsLocation) };
    auto parentDir = QString("%1%2%3%4%5").arg(QDir::homePath(), QDir::separator(), actualDir, QDir::separator(), PROGRAM_NAME);
    auto fullPath = QString("%1%2%3").arg(parentDir, QDir::separator(), "calls.sqlite3");

    if (QDir dir(parentDir); not dir.exists()) {
        dir.mkdir(parentDir);
    }

    m_db.setDatabaseName(fullPath);
}

void Database::createDB()
{
    if (not m_db.open()) {
        emit error("Database couldn't be opened, thus couldn't be created.");
        return;
    }

    QString statement = "CREATE TABLE IF NOT EXISTS Calls (\
id INTEGER PRIMARY KEY AUTOINCREMENT,\
calls INTERGER NOT NULL,\
date DATETIME NOT NULL UNIQUE,\
username TEXT NOT NULL)";

    if (not m_query.exec(statement)) {
        emit error("Database couldn't be created.");
        emit error(m_query.lastError().text());
    }

    m_db.close();
}

bool Database::open()
{
    bool result;
    if (not (result = m_db.open())) {
        emit error("Database couldn't be opened.");
    }
    return result;
}

void Database::close()
{
    m_db.close();
}

bool Database::exec(const QString& statement)
{
    bool result;
    if (not (result = m_query.exec(statement))) {
        emit error("Statement couldn't be executed.");
        emit error(m_query.lastError().text());
    }

    return result;
}

QSqlQuery& Database::query()
{
    return m_query;
}

QSqlRecord Database::record()
{
    m_query.first();
    return m_query.record();
}

QVariant Database::value(int index) const
{
    return m_query.value(index);
}
