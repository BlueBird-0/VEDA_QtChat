#include "dbmanager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QSqlRecord>
#include <QSqlError>
#include <QSqlQueryModel>
#include <QString>


bool DBManager::createConnection()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("data.db");
    if (!db.open( )) return false;

    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS user(idx INTEGER Primary Key, "
                     "id VARCHAR(20) NOT NULL, pw VARCHAR(20) NOT NULL, permission VARCHAR(20));");
    query.exec("INSERT INTO user VALUES(101, 'Yongsu', 'Kang', '1');");
    query.exec("INSERT INTO user(id, pw, permission) VALUES('Soomi', 'Kim', '1')");
    query.exec("INSERT INTO user(id, pw, permission) VALUES "
                     "('Hanmi', 'Lee', '1'), ('YoungJin', 'Suh', '1'), ('YoungHwa', 'Ryu', '1');");

    return true;
}

QSqlTableModel* DBManager::getQueryModel()
{
    return queryModel;
}

DBManager::DBManager()
{
    createConnection();
    queryModel = new QSqlTableModel();
    queryModel->setTable("user");
    queryModel->select();
}
