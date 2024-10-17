#ifndef DBMANAGER_H
#define DBMANAGER_H
class QSqlTableModel;
class QString;
class QSqlRecord;

class DBManager
{
private:
    QSqlTableModel* queryModel;
    static bool createUser(QString id, QString pw, QString permission);
    QSqlRecord searchTable(QString id);

public:
    bool checkLogin(QString id, QString pw);
    QSqlTableModel* getQueryModel();
    static bool createConnection();
    DBManager();
};

#endif // DBMANAGER_H
