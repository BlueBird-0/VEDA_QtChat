// servermanager.cpp
#include "servermanager.h"
#include "ui_servermanager.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>

serverManager::serverManager(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::serverManager)
    , tcpServer(nullptr)
{
    ui->setupUi(this);
    ui->ipEdit->setText("127.0.0.1");
    ui->portEdit->setText("5432");
    Set_tcpServer();
}

serverManager::~serverManager()
{
    clearAllConnections();
    delete tcpServer;
    delete ui;
}

void serverManager::clearAllConnections() {
    for (QTcpSocket *clientSocket : clients.keys()) {
        clientSocket->disconnectFromHost();
        clientSocket->deleteLater();
    }
    clients.clear();
    rooms.clear();
    updateClientList();
    updateRoomList();
}

void serverManager::Set_tcpServer()
{
    delete tcpServer;
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &serverManager::clientConnect);

    QHostAddress ipAddr = QHostAddress::AnyIPv4;
    quint16 port = ui->portEdit->text().toUShort();

    if(!tcpServer->listen(ipAddr, port)) {
        QMessageBox::critical(this, tr("Chat Server"),
                              tr("Unable to start the server: %1.")
                                  .arg(tcpServer->errorString()));
        close();
        return;
    }

    qDebug() << "Server is listening on all interfaces at port:" << port;
}

void serverManager::clientConnect()
{
    QTcpSocket *clientConnection = tcpServer->nextPendingConnection();
    connect(clientConnection, &QTcpSocket::disconnected, this, &serverManager::clientDisconnect);
    connect(clientConnection, &QTcpSocket::readyRead, this, &serverManager::processMessage);

    QString clientId = QString("Client_%1").arg(clientConnection->peerPort());
    clients[clientConnection] = clientId;

    qDebug() << "New connection established:" << clientId;
    updateClientList();
}

void serverManager::clientDisconnect()
{
    QTcpSocket *clientConnection = qobject_cast<QTcpSocket*>(sender());
    if (clientConnection) {
        QString clientId = clients.take(clientConnection);
        for (auto &room : rooms) {
            room.remove(clientConnection);
        }
        qDebug() << "Client disconnected:" << clientId;
        clientConnection->deleteLater();
        updateClientList();
        updateRoomList();
    }
}

void serverManager::processMessage()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket)
        return;

    QByteArray data = clientSocket->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
    QJsonObject jsonObj = jsonDoc.object();

    QString action = jsonObj["action"].toString();
    QString roomName = jsonObj["room"].toString();
    QString message = jsonObj["message"].toString();

    if (action == "create_room") {
        createRoom(clientSocket, roomName);
    } else if (action == "join_room") {
        joinRoom(clientSocket, roomName);
    } else if (action == "leave_room") {
        leaveRoom(clientSocket, roomName);
    } else if (action == "send_message") {
        sendMessageToRoom(roomName, message, clientSocket);
    }
}

void serverManager::createRoom(QTcpSocket* client, const QString& roomName)
{
    if (!rooms.contains(roomName)) {
        rooms[roomName] = QSet<QTcpSocket*>();
        rooms[roomName].insert(client);
        updateRoomList();

        QJsonObject response;
        response["action"] = "room_created";
        response["room"] = roomName;
        client->write(QJsonDocument(response).toJson());
    } else {
        QJsonObject response;
        response["action"] = "error";
        response["message"] = "Room already exists";
        client->write(QJsonDocument(response).toJson());
    }
}

void serverManager::joinRoom(QTcpSocket* client, const QString& roomName)
{
    if (rooms.contains(roomName)) {
        rooms[roomName].insert(client);
        updateRoomList();

        QJsonObject response;
        response["action"] = "joined_room";
        response["room"] = roomName;
        client->write(QJsonDocument(response).toJson());
    } else {
        QJsonObject response;
        response["action"] = "error";
        response["message"] = "Room does not exist";
        client->write(QJsonDocument(response).toJson());
    }
}

void serverManager::leaveRoom(QTcpSocket* client, const QString& roomName)
{
    if (rooms.contains(roomName)) {
        rooms[roomName].remove(client);
        if (rooms[roomName].isEmpty()) {
            rooms.remove(roomName);
        }
        updateRoomList();

        QJsonObject response;
        response["action"] = "left_room";
        response["room"] = roomName;
        client->write(QJsonDocument(response).toJson());
    }
}

void serverManager::sendMessageToRoom(const QString& roomName, const QString& message, QTcpSocket* sender)
{
    if (rooms.contains(roomName) && rooms[roomName].contains(sender)) {
        QJsonObject messageObj;
        messageObj["action"] = "new_message";
        messageObj["room"] = roomName;
        messageObj["sender"] = clients[sender];
        messageObj["message"] = message;

        QByteArray messageData = QJsonDocument(messageObj).toJson();

        for (QTcpSocket* client : rooms[roomName]) {
            if (client != sender) {
                client->write(messageData);
            }
        }
    }
}

void serverManager::on_pushButton_clicked()
{
    clearAllConnections();
    Set_tcpServer();
}

void serverManager::updateClientList()
{
    ui->clientListWidget->clear();
    for (const QString &clientId : clients.values()) {
        ui->clientListWidget->addItem(clientId);
    }
}

void serverManager::updateRoomList()
{
    ui->roomListWidget->clear();
    for (const QString &roomName : rooms.keys()) {
        ui->roomListWidget->addItem(QString("%1 (%2)").arg(roomName).arg(rooms[roomName].size()));
    }
}
