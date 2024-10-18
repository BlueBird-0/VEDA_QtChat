// tcpclient.cpp
#include "tcpclient.h"
#include "ui_tcpclient.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>

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
}

void TcpClient::on_sendButton_clicked()
{
    if(socket->state() == QAbstractSocket::ConnectedState && !currentRoom.isEmpty()) {
        QString message = ui->messageEdit->text();
        QJsonObject jsonObj;
        jsonObj["action"] = "send_message";
        jsonObj["room"] = currentRoom;
        jsonObj["message"] = message;
        sendJson(jsonObj);
        ui->messageEdit->clear();
    } else {
        QMessageBox::warning(this, "Warning", "Not connected to server or not in a room");
    }
}

void TcpClient::on_createRoomButton_clicked()
{
    QString roomName = ui->roomNameEdit->text();
    if(!roomName.isEmpty()) {
        QJsonObject jsonObj;
        jsonObj["action"] = "create_room";
        jsonObj["room"] = roomName;
        sendJson(jsonObj);
    }
}

void TcpClient::on_joinRoomButton_clicked()
{
    QString roomName = ui->roomNameEdit->text();
    if(!roomName.isEmpty()) {
        QJsonObject jsonObj;
        jsonObj["action"] = "join_room";
        jsonObj["room"] = roomName;
        sendJson(jsonObj);
    }
}

void TcpClient::on_leaveRoomButton_clicked()
{
    if(!currentRoom.isEmpty()) {
        QJsonObject jsonObj;
        jsonObj["action"] = "leave_room";
        jsonObj["room"] = currentRoom;
        sendJson(jsonObj);
    }
}

void TcpClient::onReadyRead()
{
    QByteArray data = socket->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
    QJsonObject jsonObj = jsonDoc.object();

    QString action = jsonObj["action"].toString();

    if (action == "room_created" || action == "joined_room") {
        currentRoom = jsonObj["room"].toString();
        ui->chatDisplay->appendPlainText(QString("Entered room: %1").arg(currentRoom));
    } else if (action == "left_room") {
        ui->chatDisplay->appendPlainText(QString("Left room: %1").arg(currentRoom));
        currentRoom.clear();
    } else if (action == "new_message") {
        QString sender = jsonObj["sender"].toString();
        QString message = jsonObj["message"].toString();
        ui->chatDisplay->appendPlainText(QString("%1: %2").arg(sender, message));
    } else if (action == "error") {
        QString errorMessage = jsonObj["message"].toString();
        QMessageBox::warning(this, "Error", errorMessage);
    }
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
    currentRoom.clear();
}

void TcpClient::sendJson(const QJsonObject &jsonObj)
{
    QJsonDocument jsonDoc(jsonObj);
    socket->write(jsonDoc.toJson());
}
