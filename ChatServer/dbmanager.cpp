#include "dbmanager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QSqlRecord>
#include <QSqlError>
#include <QSqlQueryModel>
#include <QString>
#include <QDebug>
using namespace std;

bool DBManager::initDB()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("data.db");
    if (!db.open( )) return false;

    deleteTable(QString("user"));//TODO : test code.
    deleteTable(QString("message"));//TODO : test code.
    deleteTable(QString("room"));//TODO : test code.

    QSqlQuery query;
    if (!isTableExists(QString("user"))){
        if (!query.exec("CREATE TABLE IF NOT EXISTS user ("
                    "idx INTEGER PRIMARY KEY,"
                    "id VARCHAR(20) NOT NULL,"
                    "pw VARCHAR(20) NOT NULL,"
                    "permission VARCHAR(20));")) {
            qDebug() << "Error creating user table:" << query.lastError().text();
        }

        // Test Account
        insertUser(QString("root"), QString("1q2w3e4r!"), QString("1"));
        insertUser(QString("admin"), QString("1q2w3e4r!"), QString("1"));
        insertUser(QString("iam"), QString("aboy"), QString("1"));
        insertUser(QString("user1"), QString("pass1"), QString("1"));
        insertUser(QString("user2"), QString("pass2"), QString("1"));
    }

    if(!isTableExists(QString("message"))){
        if (!query.exec("CREATE TABLE IF NOT EXISTS message ("
                        "message_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                        "room VARCHAR(20),"
                        "time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
                        "sender VARCHAR(20),"
                        "message VARCHAR(255));" )) {
            qDebug() << "Error creating message table:" << query.lastError().text();
        }
    }

    if(!isTableExists(QString("room"))){
        if (!query.exec("CREATE TABLE IF NOT EXISTS room ("
                        "room_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                        "room_name VARCHAR(40),"
                        "create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP);" )) {
            qDebug() << "Error creating room table:" << query.lastError().text();
        }
        insertRoom("testRoom1");    //TODO: testcode
        insertRoom("testRoom2");    //TODO: testcode
        insertRoom("testRoom3");    //TODO: testcode
    }

    return true;
}

bool DBManager::insertMessage(const QString& room, const QString& sender, const QString& message)
{
    QSqlQuery query;

    // Prepare the SQL statement to insert a new message
    query.prepare("INSERT INTO message (room, sender, message) VALUES (:room, :sender, :message)");

    // Bind the parameters
    query.bindValue(":room", room);
    query.bindValue(":sender", sender);
    query.bindValue(":message", message);

    // Execute the query and check for success
    if (!query.exec()) {
        qDebug() << "Error adding message:" << query.lastError().text();
        return false;
    }

    return true;
}

bool DBManager::insertUser(QString id, QString pw, QString permission)
{
    QString insertQuery = QString("INSERT INTO user(id, pw, permission) VALUES('%1', '%2', '%3')")
            .arg(id)
            .arg(pw)
            .arg(permission);

    QSqlQuery query;
    return query.exec( insertQuery);
}

bool DBManager::insertRoom(const QString roomName)
{
    QString insertQuery = QString("INSERT INTO room(room_name) VALUES('%1')")
            .arg(roomName);

    QSqlQuery query;
    return query.exec( insertQuery);
}

bool DBManager::isTableExists(const QString& tableName)
{
    QSqlQuery query;

    // Prepare the SQL statement to check for the existence of the table
    query.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name = :tableName;");
    query.bindValue(":tableName", tableName);

    // Execute the query
    if (!query.exec()) {
        qDebug() << "Error checking for table existence:" << query.lastError().text();
        return false;
    }

    // Return true if the table exists
    return query.next(); // If a row is returned, the table exists
}

bool DBManager::deleteTable(const QString& tableName)
{
    QSqlQuery query;

    // Construct the SQL statement directly
    QString sql = QString("DROP TABLE IF EXISTS %1;").arg(tableName);

    // Execute the query
    if (!query.exec(sql)) {
        qDebug() << "Error deleting table" << tableName << ":" << query.lastError().text();
        return false;
    }

    return true;
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
QSqlTableModel* DBManager::getMessageQueryModel()
{
    return messageQueryModel;
}

QSqlTableModel* DBManager::memoryGetMessagesByRoomId(const QString& roomId)
{
    // Create a new QSqlTableModel
    QSqlTableModel* model = new QSqlTableModel(nullptr, QSqlDatabase::database());

    // Set the table name
    model->setTable("message");

    // Apply the filter for the specified roomId
    model->setFilter(QString("room = '%1'").arg(roomId));

    // Select the data from the table
    model->select();

    // Optionally, you could check for errors here
    if (model->lastError().isValid()) {
        qDebug() << "Error retrieving messages:" << model->lastError().text();
    }

    return model; // Return the model containing the messages
}

DBManager::DBManager()
{
    initDB();
    queryModel = new QSqlTableModel();
    queryModel->setTable("user");
    queryModel->select();

    messageQueryModel = new QSqlTableModel();
    messageQueryModel->setTable("message");
    messageQueryModel->select();

}
