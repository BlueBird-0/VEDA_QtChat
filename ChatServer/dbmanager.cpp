#include "dbmanager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QSqlRecord>
#include <QSqlError>
#include <QSqlQueryModel>
#include <QString>


bool DBManager::initDB()
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
    createUser(QString("user1"), QString("pass1"), QString("1"));
    createUser(QString("user2"), QString("pass2"), QString("1"));

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

QSqlRecord DBManager::searchTable(QString id)
{
    QSqlQueryModel model;
    // Prepare the SQL query with a WHERE clause
    model.setQuery(QString("SELECT * FROM user WHERE ID = '%1'").arg(id));

    if (model.rowCount() != 0) {
        return model.record(0);
    }

    // Return an empty record if no match is found
    return QSqlRecord();

    //QString testID = model.record(4).value("ID").toString();
    //qDebug() << testID << "(testID)\n";
    //if(model.rowCount() != 0){
    //   return model.record(0);
    // }
}

bool DBManager::checkLogin(QString id, QString pw)
{
    QSqlRecord record = searchTable(id);
    if(record.isEmpty()){
        return false;
    }else {
        if( record.value("PW").toString() == pw )
            return true;    //login Success.
    }
    return false;
}

QSqlTableModel* DBManager::getQueryModel()
{
    return queryModel;
}

DBManager::DBManager()
{
    initDB();
    queryModel = new QSqlTableModel();
    queryModel->setTable("user");
    queryModel->select();
}
