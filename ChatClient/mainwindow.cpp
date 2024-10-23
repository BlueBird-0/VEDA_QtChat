#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tcpclient.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    tcp = new TcpClient(this);
    tcp->show();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::sendServer(const QByteArray &data)
{
    tcp->socket->write(data);
}
