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
    createUser(QString("root"), QString("1q2w3e4r!"), QString("1"));
    createUser(QString("admin"), QString("1q2w3e4r!"), QString("1"));
    createUser(QString("iam"), QString("aboy"), QString("1"));

    return true;
}

bool DBManager::createUser(QString id, QString pw, QString permission)
{
    QString insertQuery = QString("INSERT INTO user(id, pw, permission) VALUES('%1', '%2', '%3')")
            .arg(id)
            .arg(pw)
            .arg(permission);

    QSqlQuery query;
    return query.exec( insertQuery);
}

QRecord DBManager::searchTable(QString id){
    QSqlQueryModel model;
    model.setQuery("SELECT * FROM user");   //TODO : need make sql use where keyword

    //QString testID = model.record(4).value("ID").toString();
    //qDebug() << testID << "(testID)\n";
    if(model.rowCount() != 0){
        return model.record(0);
    }

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
