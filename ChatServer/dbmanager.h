#ifndef DBMANAGER_H
#define DBMANAGER_H
class QSqlTableModel;

class DBManager
{
private:
    QSqlTableModel* queryModel;

public:
    QSqlTableModel* getQueryModel();
    static bool createConnection();
    DBManager();
};

#endif // DBMANAGER_H
