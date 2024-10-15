#include "mainwindow.h"
#include <QApplication>
#include <QTableView>
#include <QSqlQueryModel>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlTableModel>

static bool createConnection()
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
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    createConnection();
    QSqlTableModel queryModel;
    queryModel.setTable("user");
    queryModel.select( );

    queryModel.setHeaderData(0, Qt::Horizontal, QObject::tr("IDX"));
    queryModel.setHeaderData(1, Qt::Horizontal, QObject::tr("ID"));
    queryModel.setHeaderData(2, Qt::Horizontal, QObject::tr("PW"));
    queryModel.setHeaderData(3, Qt::Horizontal, QObject::tr("Permission"));

    QTableView *tableview = new QTableView;
    tableview->setModel(&queryModel);
    tableview->setWindowTitle(QObject::tr("Query Model"));
    tableview->show( );
    MainWindow w;
    w.show();
    return a.exec();
}
