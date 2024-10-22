// servermanager.cpp
#include "servermanager.h"
#include "ui_servermanager.h"
#include <QMessageBox>
#include <mainwindow.h>
#include <QSqlTableModel>
#include <QTableView>
#include <message.h>

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
        QString clientId = clients[clientSocket];

        Message recvMsg(data);
        qDebug() <<"["<< recvMsg.senderId<<" : "<< recvMsg.roomName << "]"<<recvMsg.messageType<<"-"<<recvMsg.message;

        if( recvMsg.messageType == MessageType::Login){
            QString username(recvMsg.senderId);
            QString password(recvMsg.message);
            handleLogin(clientSocket, username, password);
        } else if (recvMsg.messageType == MessageType::create_Room) {
            createRoom(clientSocket, recvMsg.message);
        } else if (recvMsg.messageType == MessageType::join_Room) {
            joinRoom(clientSocket, recvMsg.message);
        } else if (recvMsg.messageType == MessageType::left_Room) {
            leaveRoom(clientSocket, recvMsg.roomName);
        } else if (recvMsg.messageType == MessageType::send_Message) {
            QString roomName = recvMsg.roomName;
            QString message = recvMsg.message;
            dbManager->insertMessage(roomName, clients[clientSocket], message);

            sendMessageToRoom(roomName, message, clientSocket);
        }

        // Log the received message
        qDebug() << "Received from" << clientId << ":" << QString::fromUtf8(data);

}

void serverManager::handleLogin(QTcpSocket* client, const QString& username, const QString& password)
{
    Message ackMsg;
    ackMsg.SetSenderId(username);
    ackMsg.SetMessageType(MessageType::Login);
    bool loginSuccess = dbManager->checkLogin(username, password);
    if(loginSuccess){
        ackMsg.SetMessage(QString("Success"));
        ui->logTextEdit->appendPlainText(QString("User logged in: %1").arg(username));
        clients[client] = username;
        updateClientList();
    }else{
        ackMsg.SetMessage(QString("Fail"));
        ui->logTextEdit->appendPlainText(QString("Failed login attempt for username: %1").arg(username));
    }
    sendMessage(*client, ackMsg);
}

void serverManager::createRoom(QTcpSocket* client, const QString& roomName)
{
    if (!clients.contains(client)) {
        Message ackMsg;
        ackMsg.SetMessageType(MessageType::Error);
        ackMsg.SetMessage(QString("You must be logged in to create a room"));
        sendMessage(*client, ackMsg);
        return;
    }

    if (!rooms.contains(roomName)) {
        rooms[roomName] = QSet<QTcpSocket*>();
        rooms[roomName].insert(client);
        updateRoomList();

        Message ackMsg;
        ackMsg.SetMessageType(MessageType::create_Room);
        ackMsg.SetMessage(roomName);
        sendMessage(*client, ackMsg);

        dbManager->insertRoom(roomName);
        ui->logTextEdit->appendPlainText(QString("Room created: %1").arg(roomName));
    } else {
        Message ackMsg;
        ackMsg.SetMessageType(MessageType::Error);
        ackMsg.SetMessage(QString("Room already exists"));
        sendMessage(*client, ackMsg);
    }
}

void serverManager::joinRoom(QTcpSocket* client, const QString& roomName)
{
    if (!clients.contains(client)) {
        Message ackMsg;
        ackMsg.SetMessageType(MessageType::Error);
        ackMsg.SetMessage(QString("You must be logged in to join a room"));
        sendMessage(*client, ackMsg);
        return;
    }

    if (rooms.contains(roomName)) {
        rooms[roomName].insert(client);
        updateRoomList();

        Message ackMsg;
        ackMsg.SetMessageType(MessageType::join_Room);
        ackMsg.SetMessage(roomName);
        sendMessage(*client, ackMsg);

        //send previous Messages
        sendPrevMessagesRoomToClient(roomName, client);

        ui->logTextEdit->appendPlainText(QString("User %1 joined room: %2").arg(clients[client], roomName));
    } else {
        Message ackMsg;
        ackMsg.SetMessageType(MessageType::Error);
        ackMsg.SetMessage(QString("Room does not exist"));
        sendMessage(*client, ackMsg);
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

        Message response;
        response.SetMessageType(MessageType::left_Room);
        response.SetMessage(QString("Room does not exist"));
        sendMessage(*client, response);

        ui->logTextEdit->appendPlainText(QString("User %1 left room: %2").arg(clients[client], roomName));
    }
}

void serverManager::sendMessageToRoom(const QString& roomName, const QString& message, QTcpSocket* sender)
{
    if (rooms.contains(roomName) && rooms[roomName].contains(sender)) {
        Message ackMsg;
        ackMsg.SetMessageType(MessageType::new_Message);
        ackMsg.SetRoomName(roomName);
        ackMsg.SetSenderId(clients[sender]);
        ackMsg.SetMessage(message);

        for (QTcpSocket* client : rooms[roomName]) {
            if (client != sender) {
                sendMessage(*client, ackMsg);
            }
        }

        ui->logTextEdit->appendPlainText(QString("Message in %1 from %2: %3").arg(roomName, clients[sender], message));
    }
}

void serverManager::sendMessageToClient(const QString& roomName, const QString& message, const QString& senderStr, QTcpSocket* client)
{
    Message ackMsg;
    ackMsg.SetMessageType(MessageType::new_Message);
    ackMsg.SetRoomName(roomName);
    ackMsg.SetSenderId(senderStr);
    ackMsg.SetMessage(message);
    sendMessage(*client, ackMsg);
    qDebug() << "sendMessageMessage:"<< ackMsg.senderId<<" "<< ackMsg.message;


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

        sendMessageToClient(room, message, sender, client);
    }
    delete model;
}


//send message for clients.
void serverManager::sendMessage(QTcpSocket& client, Message& message)
{
    vector<QTcpSocket*> clients;
    clients.push_back(&client);
    vector<Message*> messages;
    messages.push_back(&message);
    sendMessage(clients, messages);
}

void serverManager::sendMessage(vector<QTcpSocket*> &clients, vector<Message*> &messageList)
{
    QByteArray packet;
    for(auto msg: messageList){
        packet.append(msg->getByteArray());
        packet.append("&&");
    }

    for(auto client : clients){
        client->write(packet);
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
