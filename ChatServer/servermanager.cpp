#include "servermanager.h"
#include "ui_servermanager.h"
#include <QMessageBox>

serverManager::serverManager(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::serverManager)
{
    ui->setupUi(this);

    ui->ipEdit->setText("127.0.0.1");  //TODO : 나중에 IP 바꿀수 있도록 기능추가 필요
    ui->portEdit->setText("5432");  //TODO : 나중에 포트 바꿀수 있도록 기능추가 필요

    Set_tcpServer();
}

void serverManager::Set_tcpServer()
{
    delete tcpServer;
    tcpServer = new QTcpServer(this);
    connect(tcpServer, SIGNAL(newConnection()), SLOT(clientConnect()));

    QHostAddress ipAddr = QHostAddress(ui->ipEdit->toPlainText());
    quint16 port = ui->portEdit->toPlainText().toUShort();
    if(!tcpServer->listen(ipAddr, port)){
        QMessageBox::critical(this, tr("Echo Server"), \
                                                       tr("Unable to start the server: %1.") \
                                                           .arg(tcpServer->errorString()));
        close();
        return;
    }
}

void serverManager::clientConnect()
{
    QTcpSocket *clientConnection = tcpServer->nextPendingConnection();
    connect(clientConnection, SIGNAL(disconnected( )), clientConnection, SLOT(deleteLater( )));
    connect(clientConnection, SIGNAL(readyRead( )), SLOT(echoData( )));
    qDebug()<<"new connection is established...";
}

serverManager::~serverManager()
{
    delete ui;
}

void serverManager::on_pushButton_clicked()
{
    Set_tcpServer();
}

