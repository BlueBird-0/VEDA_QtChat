// servermanager.cpp
#include "servermanager.h"
#include "ui_servermanager.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <mainwindow.h>
#include <QSqlTableModel>
#include <QTableView>
using namespace std;

serverManager::serverManager(QWidget *parent)
    : QWidget(parent)
    , dbManager(new DBManager())
    , ui(new Ui::serverManager)
    , tcpServer(nullptr)
{
    ui->setupUi(this);
    ui->ipEdit->setText("127.0.0.1");
    ui->portEdit->setText("5432");
    Set_tcpServer();


    queryModel = dbManager->getQueryModel();
    queryModel->setHeaderData(0, Qt::Horizontal, QObject::tr("IDX"));
    queryModel->setHeaderData(1, Qt::Horizontal, QObject::tr("ID"));
    queryModel->setHeaderData(2, Qt::Horizontal, QObject::tr("PW"));
    queryModel->setHeaderData(3, Qt::Horizontal, QObject::tr("Permission"));

    tableview = new QTableView();
    tableview->setModel(queryModel);
    tableview->setWindowTitle(QObject::tr("DB_user table"));
    //tableview->show( );


    // message Table Test
    messageQueryModel = dbManager->getMessageQueryModel();
    messageQueryModel->setHeaderData(0, Qt::Horizontal, QObject::tr("aa"));
    messageQueryModel->setHeaderData(1, Qt::Horizontal, QObject::tr("bb"));
    messageQueryModel->setHeaderData(2, Qt::Horizontal, QObject::tr("cc"));
    messageQueryModel->setHeaderData(3, Qt::Horizontal, QObject::tr("dd"));

    messageTableView = new QTableView();
    messageTableView->setModel(messageQueryModel);
    messageTableView->setWindowTitle(QObject::tr("DB_message table"));
    //messageTableView->show( );

    // Add some dummy user credentials (in a real app, you'd use a database)
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
        dbManager->insertMessage(roomName, clients[clientSocket], message);

        sendMessageToRoom(roomName, message, clientSocket);
    }
}

void serverManager::handleLogin(QTcpSocket* client, const QString& username, const QString& password)
{
    QJsonObject response;
    response["action"] = "login_response";

    bool loginSuccess = dbManager->checkLogin(username, password);
    if (loginSuccess) {
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

void serverManager::createRoom(QTcpSocket* client, const QString& roomName)
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
        dbManager->insertRoom(roomName);
        ui->logTextEdit->appendPlainText(QString("Room created: %1").arg(roomName));
    } else {
        QJsonObject response;
        response["action"] = "error";
        response["message"] = "Room already exists";
        client->write(QJsonDocument(response).toJson());
    }
}

void serverManager::joinRoom(QTcpSocket* client, const QString& roomName)
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

        //send previous Messages
        //sendPrevMessagesRoomToClient(roomName, client);

        ui->logTextEdit->appendPlainText(QString("User %1 joined room: %2").arg(clients[client], roomName));
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

        ui->logTextEdit->appendPlainText(QString("User %1 left room: %2").arg(clients[client], roomName));
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

        ui->logTextEdit->appendPlainText(QString("Message in %1 from %2: %3").arg(roomName, clients[sender], message));
    }
}

void serverManager::sendMessageToClient(const QString& roomName, const QString& message, const QString& senderStr, QTcpSocket* client)
{
    QJsonObject messageObj;
    messageObj["action"] = "new_message";
//    messageObj["time"] = time;
    messageObj["sender"] = senderStr;
    messageObj["message"] = message;

    client->write(QJsonDocument(messageObj).toJson());
    qDebug() << "sendMessageMessage:"<< messageObj["action"] <<" "<< roomName<<" "<< time<<" "<< messageObj["sender"]<<" "<<messageObj["message"];

    ui->logTextEdit->appendPlainText(QString("Message in %1 from %2: %3").arg(roomName, senderStr, message));
}

void serverManager::sendPrevMessagesRoomToClient(const QString &roomName, QTcpSocket* client)
{
    QSqlTableModel* model = dbManager->memoryGetMessagesByRoomId(roomName);

    // Iterate through the rows of the model
    for (int row = 0; row < model->rowCount(); ++row) {
        int message_id = model->data(model->index(row, 0)).toInt();
        QString room = model->data(model->index(row, 1)).toString();
        QString time = model->data(model->index(row, 2)).toString();
        QString sender = model->data(model->index(row, 3)).toString();
        QString message = model->data(model->index(row, 4)).toString();

        //sendMessageToClient(room, message, sender, client);
    }
    delete model;
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
