#ifndef DBMANAGER_H
#define DBMANAGER_H
class QSqlTableModel;
class QString;
class QSqlRecord;

class DBManager
{
private:
    QSqlTableModel* queryModel;
    QSqlTableModel* messageQueryModel;
    QSqlRecord searchTable(QString id);

public:
    DBManager();
    static bool initDB();
    bool checkLogin(QString id, QString pw);

    static bool insertUser(QString id, QString pw, QString permission);
    static bool insertRoom(const QString roomName);
    static bool insertMessage(const QString& room, const QString& sender, const QString& message);

    QSqlTableModel* getQueryModel();
    QSqlTableModel* getMessageQueryModel();

    static bool isTableExists(const QString& tableName);
    static bool deleteTable(const QString& tableName);
    QSqlTableModel* memoryGetMessagesByRoomId(const QString& roomId);
};

#endif // DBMANAGER_H
