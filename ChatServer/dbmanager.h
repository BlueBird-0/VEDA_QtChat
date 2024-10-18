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
    static bool createUser(QString id, QString pw, QString permission);
    QSqlRecord searchTable(QString id);

public:
    DBManager();
    static bool initDB();
    bool checkLogin(QString id, QString pw);
    QSqlTableModel* getQueryModel();
    QSqlTableModel* getMessageQueryModel();

    static bool isTableExists(const QString& tableName);
    static bool deleteTable(const QString& tableName);
    static bool initMessageTable();
    static bool addMessage(const QString& room, const QString& sender, const QString& message);
};

#endif // DBMANAGER_H
