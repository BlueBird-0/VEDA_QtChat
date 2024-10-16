#ifndef DBMANAGER_H
#define DBMANAGER_H
class QSqlTableModel;
class QString;

class DBManager
{
private:
    QSqlTableModel* queryModel;
    static bool createUser(QString id, QString pw, QString permission);

public:
    QSqlTableModel* getQueryModel();
    static bool createConnection();
    DBManager();
};

#endif // DBMANAGER_H
