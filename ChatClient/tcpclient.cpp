#include "tcpclient.h"
#include "ui_tcpclient.h"
#include <QMessageBox>

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
    ui->chatDisplay->appendPlainText("Server: " + QString::fromUtf8(data));
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
