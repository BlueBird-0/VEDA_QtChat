// servermanager.cpp
#include "servermanager.h"
#include "ui_servermanager.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QDir>
#include <QUuid>

ServerManager::ServerManager(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ServerManager)
    , tcpServer(nullptr)
{
    ui->setupUi(this);
    ui->ipEdit->setText("127.0.0.1");
    ui->portEdit->setText("5432");
    Set_tcpServer();
    // Add some dummy user credentials (in a real app, you'd use a database)
    userCredentials["user1"] = "pass1";
    userCredentials["user2"] = "pass2";

    // Create a directory for file storage
    QDir().mkdir("file_storage");
}

ServerManager::~ServerManager()
{
    clearAllConnections();
    delete tcpServer;
    delete ui;
}

void ServerManager::clearAllConnections() {
    for (QTcpSocket *clientSocket : clients.keys()) {
        clientSocket->disconnectFromHost();
        clientSocket->deleteLater();
    }
    clients.clear();
    rooms.clear();
    updateClientList();
    updateRoomList();
}

void ServerManager::Set_tcpServer()
{
    delete tcpServer;
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &ServerManager::clientConnect);

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

void ServerManager::clientConnect()
{
    QTcpSocket *clientConnection = tcpServer->nextPendingConnection();
    connect(clientConnection, &QTcpSocket::disconnected, this, &ServerManager::clientDisconnect);
    connect(clientConnection, &QTcpSocket::readyRead, this, &ServerManager::processMessage);

    QString clientId = QString("Client_%1").arg(clientConnection->peerPort());
    clients[clientConnection] = clientId;

    qDebug() << "New connection established:" << clientId;
    updateClientList();
}

void ServerManager::clientDisconnect()
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

void ServerManager::processMessage()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket)
        return;

    QByteArray data = clientSocket->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
    QJsonObject jsonObj = jsonDoc.object();

    QString action = jsonObj["action"].toString();

    qDebug() << action;
    if (action == "login") {
        QString username = jsonObj["username"].toString();
        QString password = jsonObj["password"].toString();
        handleLogin(clientSocket, username, password);
    } else if (action == "create_room") {
        createRoom(clientSocket, jsonObj["room"].toString());
    } else if (action == "join_room") {
        joinRoom(clientSocket, jsonObj["room"].toString());
    } else if (action == "leave_room") {
        leaveRoom(clientSocket, jsonObj["room"].toString());
    } else if (action == "send_message") {
        QString roomName = jsonObj["room"].toString();
        QString message = jsonObj["message"].toString();
        sendMessageToRoom(roomName, message, clientSocket);
    } else if (action == "init_file_upload") {
        handleFileUpload(clientSocket, jsonObj);
    } else if (action == "upload_file") {
        QString roomName = jsonObj["room"].toString();
        QString fileId = jsonObj["fileId"].toString();
        QString fileName = jsonObj["filename"].toString();
        sendFileToRoom(roomName, fileId, fileName, clientSocket);
    } else if (action == "request_file") {
        QString fileId = jsonObj["fileId"].toString();
        handleFileDownloadRequest(clientSocket, fileId);
    }
}

bool ServerManager::authenticateUser(const QString& username, const QString& password)
{
    return userCredentials.contains(username) && userCredentials[username] == password;
}

void ServerManager::handleLogin(QTcpSocket* client, const QString& username, const QString& password)
{
    QJsonObject response;
    response["action"] = "login_response";

    if (authenticateUser(username, password)) {
        clients[client] = username;
        response["success"] = true;
        response["message"] = "Login successful";
        ui->logTextEdit->appendPlainText(QString("User logged in: %1").arg(username));
        updateClientList();
    } else {
        response["success"] = false;
        response["message"] = "Invalid username or password";
        ui->logTextEdit->appendPlainText(QString("Failed login attempt for username: %1").arg(username));
    }

    client->write(QJsonDocument(response).toJson());
}

void ServerManager::createRoom(QTcpSocket* client, const QString& roomName)
{
    if (!clients.contains(client)) {
        QJsonObject response;
        response["action"] = "error";
        response["message"] = "You must be logged in to create a room";
        client->write(QJsonDocument(response).toJson());
        return;
    }

    if (!rooms.contains(roomName)) {
        rooms[roomName] = QSet<QTcpSocket*>();
        rooms[roomName].insert(client);
        updateRoomList();

        QJsonObject response;
        response["action"] = "room_created";
        response["room"] = roomName;
        client->write(QJsonDocument(response).toJson());

        ui->logTextEdit->appendPlainText(QString("Room created: %1").arg(roomName));
    } else {
        QJsonObject response;
        response["action"] = "error";
        response["message"] = "Room already exists";
        client->write(QJsonDocument(response).toJson());
    }
}

void ServerManager::joinRoom(QTcpSocket* client, const QString& roomName)
{
    if (!clients.contains(client)) {
        QJsonObject response;
        response["action"] = "error";
        response["message"] = "You must be logged in to join a room";
        client->write(QJsonDocument(response).toJson());
        return;
    }

    if (rooms.contains(roomName)) {
        rooms[roomName].insert(client);
        updateRoomList();

        QJsonObject response;
        response["action"] = "joined_room";
        response["room"] = roomName;
        client->write(QJsonDocument(response).toJson());

        ui->logTextEdit->appendPlainText(QString("User %1 joined room: %2").arg(clients[client], roomName));
    } else {
        QJsonObject response;
        response["action"] = "error";
        response["message"] = "Room does not exist";
        client->write(QJsonDocument(response).toJson());
    }
}

void ServerManager::leaveRoom(QTcpSocket* client, const QString& roomName)
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

        ui->logTextEdit->appendPlainText(QString("User %1 left room: %2").arg(clients[client], roomName));
    }
}

void ServerManager::sendMessageToRoom(const QString& roomName, const QString& message, QTcpSocket* sender)
{
    if (rooms.contains(roomName) && rooms[roomName].contains(sender)) {
        QJsonObject messageObj;
        messageObj["action"] = "new_message";
        messageObj["room"] = roomName;
        messageObj["sender"] = clients[sender];
        messageObj["message"] = message;

        QByteArray messageData = QJsonDocument(messageObj).toJson();

        for (QTcpSocket* client : rooms[roomName]) {
            if (client != sender)
                client->write(messageData);
        }

        ui->logTextEdit->appendPlainText(QString("Message in %1 from %2: %3").arg(roomName, clients[sender], message));
    }
}

void ServerManager::handleFileUpload(QTcpSocket* sender, const QJsonObject& fileInfo)
{
    QString fileName = fileInfo["filename"].toString();
    qint64 fileSize = fileInfo["filesize"].toString().toLongLong();
    QString mimeType = fileInfo["mimetype"].toString();
    QString roomName = fileInfo["room"].toString();
    QString base64Data = fileInfo["data"].toString();
    QByteArray fileData = QByteArray::fromBase64(base64Data.toUtf8());

    QString fileId = generateUniqueFileId();
    QString filePath = QString("file_storage/%1").arg(fileId);

    qDebug() << fileName << fileSize << mimeType << roomName << fileId << filePath;

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        fileStorage[fileId] = filePath;
        file.write(fileData);
        file.close();

        qDebug() << "file written successfully on " << filePath;

        QJsonObject response;
        response["action"] = "file_shared";
        response["sender"] = clients[sender];
        response["fileId"] = fileId;
        response["fileName"] = fileName;

        for (QTcpSocket* client : rooms[roomName]) {
            if (client != sender)
                client->write(QJsonDocument(response).toJson());
        }

        ui->logTextEdit->appendPlainText(QString("File upload initiated: %1 (Size: %2 bytes, Type: %3)")
                                             .arg(fileName)
                                             .arg(fileSize)
                                             .arg(mimeType));
    } else {
        QJsonObject response;
        response["action"] = "error";
        response["message"] = "Failed to prepare for file upload";
        sender->write(QJsonDocument(response).toJson());
    }
}

void ServerManager::sendFileToRoom(const QString& roomName, const QString& fileId, const QString& fileName, QTcpSocket* sender)
{
    if (rooms.contains(roomName) && rooms[roomName].contains(sender)) {
        QJsonObject fileObj;
        fileObj["action"] = "file_shared";
        fileObj["room"] = roomName;
        fileObj["sender"] = clients[sender];
        fileObj["fileId"] = fileId;
        fileObj["filename"] = fileName;

        QByteArray fileData = QJsonDocument(fileObj).toJson();

        for (QTcpSocket* client : rooms[roomName]) {
            client->write(fileData);
        }

        ui->logTextEdit->appendPlainText(QString("File shared in %1 from %2: %3 (ID: %4)")
                                             .arg(roomName, clients[sender], fileName, fileId));
    }
}

void ServerManager::handleFileDownloadRequest(QTcpSocket* client, const QString& fileId)
{
    if (fileStorage.contains(fileId)) {
        QString filePath = fileStorage[fileId];
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray fileData = file.readAll();
            file.close();

            QJsonObject response;
            response["action"] = "file_data";
            response["fileId"] = fileId;
            response["filename"] = QFileInfo(filePath).fileName();
            response["data"] = QString(fileData.toBase64());

            client->write(QJsonDocument(response).toJson());

            qDebug() << QString("File downloaded by %1: %2")
                            .arg(clients[client], QFileInfo(filePath).fileName());
            ui->logTextEdit->appendPlainText(QString("File downloaded by %1: %2")
                                                 .arg(clients[client], QFileInfo(filePath).fileName()));
        } else {
            QJsonObject response;
            response["action"] = "error";
            response["message"] = "Failed to read file data";
            client->write(QJsonDocument(response).toJson());
        }
    } else {
        QJsonObject response;
        response["action"] = "error";
        response["message"] = "File not found";
        client->write(QJsonDocument(response).toJson());
    }
}

QString ServerManager::generateUniqueFileId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void ServerManager::on_pushButton_clicked()
{
    clearAllConnections();
    Set_tcpServer();
}

void ServerManager::updateClientList()
{
    ui->clientListWidget->clear();
    for (const QString &clientId : clients.values()) {
        ui->clientListWidget->addItem(clientId);
    }
}

void ServerManager::updateRoomList()
{
    ui->roomListWidget->clear();
    for (const QString &roomName : rooms.keys()) {
        ui->roomListWidget->addItem(QString("%1 (%2)").arg(roomName).arg(rooms[roomName].size()));
    }
}
