#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dbmanager.h"
#include <QSqlTableModel>
#include <QTableView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    sManager = new serverManager();
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionSetServer_triggered()
{
    sManager->show();
}

