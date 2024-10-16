#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dbmanager.h"
#include <QSqlTableModel>
#include <QTableView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    //
    QSqlTableModel* queryModel = dbManager.getQueryModel();
    queryModel->setHeaderData(0, Qt::Horizontal, QObject::tr("IDX"));
    queryModel->setHeaderData(1, Qt::Horizontal, QObject::tr("ID"));
    queryModel->setHeaderData(2, Qt::Horizontal, QObject::tr("PW"));
    queryModel->setHeaderData(3, Qt::Horizontal, QObject::tr("Permission"));

    QTableView *tableview = new QTableView;
    tableview->setModel(queryModel);
    tableview->setWindowTitle(QObject::tr("DB_user table"));
    tableview->show( );

    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionSetServer_triggered()
{
    manager.show();
}

