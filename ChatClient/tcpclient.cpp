// tcpclient.cpp
#include "tcpclient.h"
#include "ui_tcpclient.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
using namespace std;

TcpClient::TcpClient(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TcpClient),
    socket(new QTcpSocket(this)),
    isLoggedIn(false)
{
    ui->setupUi(this);

    connect(socket, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(socket, &QTcpSocket::connected, this, &TcpClient::onConnected);
    connect(socket, &QTcpSocket::disconnected, this, &TcpClient::onDisconnected);

    ui->serverIP->setText("127.0.0.1");
    ui->serverPort->setText("5432");

    updateUIState();
}

TcpClient::~TcpClient()
{
    if(socket->state() == QAbstractSocket::ConnectedState) {
        socket->disconnectFromHost();
    }
    delete ui;
}

void TcpClient::updateUIState()
{
    bool connected = socket->state() == QAbstractSocket::ConnectedState;
    ui->serverIP->setEnabled(!connected);
    ui->serverPort->setEnabled(!connected);
    ui->connectButton->setText(connected ? "Disconnect" : "Connect");

    ui->usernameEdit->setEnabled(connected && !isLoggedIn);
    ui->passwordEdit->setEnabled(connected && !isLoggedIn);
    ui->loginButton->setEnabled(connected && !isLoggedIn);

    ui->roomNameEdit->setEnabled(isLoggedIn);
    ui->createRoomButton->setEnabled(isLoggedIn);
    ui->joinRoomButton->setEnabled(isLoggedIn);
    ui->leaveRoomButton->setEnabled(isLoggedIn && !currentRoom.isEmpty());
    ui->messageEdit->setEnabled(isLoggedIn && !currentRoom.isEmpty());
    ui->sendButton->setEnabled(isLoggedIn && !currentRoom.isEmpty());
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

void TcpClient::on_loginButton_clicked()
{
    username = ui->usernameEdit->text();
    QString password = ui->passwordEdit->text();

    QJsonObject jsonObj;
    jsonObj["action"] = "login";
    jsonObj["username"] = username;
    jsonObj["password"] = password;
    sendJson(jsonObj);
}

void TcpClient::on_sendButton_clicked()
{
    if(isLoggedIn && !currentRoom.isEmpty()) {
        QString message = ui->messageEdit->text();
        QJsonObject jsonObj;
        jsonObj["action"] = "send_message";
        jsonObj["room"] = currentRoom;
        jsonObj["message"] = message;
        sendJson(jsonObj);
        ui->messageEdit->clear();
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
    qDebug() << "onReadyRead(): action[" <<action <<"]";

    if (action == "login_response") {
        bool success = jsonObj["success"].toBool();
        if (success) {
            isLoggedIn = true;
            ui->chatDisplay->appendPlainText("Logged in successfully");
        } else {
            ui->chatDisplay->appendPlainText("Login failed: " + jsonObj["message"].toString());
        }
        updateUIState();
    } else if (action == "room_created" || action == "joined_room") {
        currentRoom = jsonObj["room"].toString();
        ui->chatDisplay->appendPlainText(QString("Entered room: %1").arg(currentRoom));
        updateUIState();
    } else if (action == "left_room") {
        ui->chatDisplay->appendPlainText(QString("Left room: %1").arg(currentRoom));
        currentRoom.clear();
        updateUIState();
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
    ui->chatDisplay->appendPlainText("Connected to server");
    updateUIState();
}

void TcpClient::onDisconnected()
{
    ui->chatDisplay->appendPlainText("Disconnected from server");
    isLoggedIn = false;
    currentRoom.clear();
    updateUIState();
}

void TcpClient::sendJson(const QJsonObject &jsonObj)
{
    QJsonDocument jsonDoc(jsonObj);
    socket->write(jsonDoc.toJson());
}
