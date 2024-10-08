#include "tcpclient.h"
#include "ui_tcpclient.h"
#include <QMessageBox>
#include "message.h"
#include "logindialog.h"

TcpClient::TcpClient(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TcpClient),
    socket(new QTcpSocket(this))
{
    ui->setupUi(this);

    connect(socket, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(socket, &QTcpSocket::connected, this, &TcpClient::onConnected);
    connect(socket, &QTcpSocket::disconnected, this, &TcpClient::onDisconnected);

    ui->serverIP->setText("127.0.0.1");
    ui->serverPort->setText("5432");
}

TcpClient::~TcpClient()
{
    /* if socket connectec, disconnect */
    if(socket->state() == QAbstractSocket::ConnectedState) {
        socket->disconnectFromHost();
    }
    delete ui;
}

void TcpClient::on_connectButton_clicked()
{
    if(socket->state() == QAbstractSocket::UnconnectedState) {
        QString ip = ui->serverIP->text();
        quint16 port = ui->serverPort->text().toUShort();
        socket->connectToHost(ip, port);
    } else {
        socket->disconnectFromHost();
    }


    //login
    LoginDialog loginDialog;
    if( loginDialog.exec() == QDialog::Accepted){
        //TODO : 로그인 기능 구현필요
    }
}

void TcpClient::on_sendButton_clicked()
{
    if(socket->state() == QAbstractSocket::ConnectedState) {
        QString message = ui->messageEdit->text();
        socket->write(message.toUtf8());
        ui->messageEdit->clear();
    } else {
        QMessageBox::warning(this, "Warning", "Not connected to server");
    }
}

void TcpClient::onReadyRead()
{
    QByteArray data = socket->readAll();
    Message msg(data);

    ui->chatDisplay->appendPlainText(QString::fromUtf8(msg.senderId) + " : " + QString::fromUtf8(msg.message));
}

void TcpClient::onConnected()
{
    ui->connectButton->setText("Disconnect");
    ui->chatDisplay->appendPlainText("Connected to server");
}

void TcpClient::onDisconnected()
{
    ui->connectButton->setText("Connect");
    ui->chatDisplay->appendPlainText("Disconnected from server");
}
