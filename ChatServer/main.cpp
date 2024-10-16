#include "mainwindow.h"
#include <QApplication>
#include <QTableView>
#include <QSqlQueryModel>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <dbmanager.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    DBManager dbManager;

    //QSqlTableModel queryModel;
    //queryModel.setTable("user");
    //queryModel.select( );


    //search
    QSqlQueryModel model;
    model.setQuery("SELECT * FROM user");
    QString testID = model.record(4).value("ID").toString();
    qDebug() << testID << "(testID)\n";


    QSqlTableModel* queryModel = dbManager.getQueryModel();
    queryModel->setHeaderData(0, Qt::Horizontal, QObject::tr("IDX"));
    queryModel->setHeaderData(1, Qt::Horizontal, QObject::tr("ID"));
    queryModel->setHeaderData(2, Qt::Horizontal, QObject::tr("PW"));
    queryModel->setHeaderData(3, Qt::Horizontal, QObject::tr("Permission"));



    QTableView *tableview = new QTableView;
    //tableview->setModel(&queryModel);
    tableview->setModel(queryModel);
    tableview->setWindowTitle(QObject::tr("Query Model"));
    tableview->show( );
    MainWindow w;
    w.show();
    return a.exec();
}
