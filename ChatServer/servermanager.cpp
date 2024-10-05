#include "servermanager.h"
#include "ui_servermanager.h"
#include <QMessageBox>

serverManager::serverManager(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::serverManager)
{
    ui->setupUi(this);

    tcpServer = new QTcpServer(this);
    connect(tcpServer, SIGNAL(newConnection()), nullptr);
    if(!tcpServer->listen()){
        QMessageBox::critical(this, tr("Echo Server"), \
                                                       tr("Unable to start the server: %1.") \
                                                           .arg(tcpServer->errorString()));
        close();
        return;
    }

    ui->ipEdit->setText("127.0.0.1");  //TODO : 나중에 IP 바꿀수 있도록 기능추가 필요
    ui->portEdit->setText("5432");  //TODO : 나중에 포트 바꿀수 있도록 기능추가 필요

}

serverManager::~serverManager()
{
    delete ui;
}
