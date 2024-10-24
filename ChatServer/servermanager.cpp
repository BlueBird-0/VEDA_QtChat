// servermanager.cpp
#include "servermanager.h"
#include "ui_servermanager.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QDir>
#include <QUuid>
#include <mainwindow.h>
#include <QSqlTableModel>
#include <QTableView>
#include <message.h>
#include <QMutex>

using namespace std;

ServerManager::ServerManager(QWidget *parent)
    : QWidget(parent)
    , dbManager(new DBManager())
    , ui(new Ui::ServerManager)
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

QMutex tcpMutex;
void ServerManager::processMessage()
{
    tcpMutex.lock();
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket)
        return;

    //QByteArray data = clientSocket->readAll();
    qDebug() <<"==== size of Message: " << sizeof(Message);

    QByteArray data = clientSocket->readAll();
    qDebug() <<"recvByteSize:" << data.size();
    int msgCount=0;
    int totalSize =0;
    int size = 0;

    QList<QByteArray> list = data.split(ENDKEY);
    //qDebug() <<"message:" << data;
    while(!list.empty()){
        QByteArray *msgBytes = &list.front();
        //qDebug() <<"splitmsg:" << *msgBytes;
        if(msgBytes->size() == 0){
            list.pop_front();   //Message한개 삭제
            break;
        }

        Message recvMsg(*msgBytes);
        list.pop_front();   //Message한개 삭제

        msgCount++;
        totalSize = recvMsg.fileSize;
        size += recvMsg.messageLength;
        //qDebug() <<"messageLength:" << recvMsg.messageLength;

        QString clientId = clients[clientSocket];

        //qDebug() <<"["<< recvMsg.senderId<<" : "<< recvMsg.roomName << "]"<<recvMsg.messageType<<"-"<<recvMsg.message;

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
        } else if (recvMsg.messageType == MessageType::upload_file || recvMsg.messageType == MessageType::uploadInit_file) {
            static QByteArray fileByte;
            static int fileSize;

            if(recvMsg.messageType == MessageType::uploadInit_file) {
                fileByte.clear();
                fileSize = 0;
            }

            fileSize += recvMsg.messageLength;
            fileByte.append(recvMsg.message);
            if(fileSize == recvMsg.fileSize){
                handleFileUpload(clientSocket, recvMsg, &fileByte);
            }
        } else if (recvMsg.messageType == MessageType::request_file) {
            QString fileId = recvMsg.message;
            handleFileDownloadRequest(clientSocket, fileId);
        }
    }
    tcpMutex.unlock();
    // Log the received message
    //qDebug() << "Received from" << clientId << ":" << QString::fromUtf8(data);
}

bool ServerManager::authenticateUser(const QString& username, const QString& password)
{
    return userCredentials.contains(username) && userCredentials[username] == password;
}

void ServerManager::handleLogin(QTcpSocket* client, const QString& username, const QString& password)
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

void ServerManager::createRoom(QTcpSocket* client, const QString& roomName)
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

void ServerManager::joinRoom(QTcpSocket* client, const QString& roomName)
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

void ServerManager::leaveRoom(QTcpSocket* client, const QString& roomName)
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

void ServerManager::sendMessageToRoom(const QString& roomName, const QString& message, QTcpSocket* sender)
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

void ServerManager::handleFileUpload(QTcpSocket* sender, const Message& fileInfo, const QByteArray *fileByte)
{
    QString fileName = fileInfo.fileName;
    qint64 fileSize = fileInfo.fileSize;
    QString mimeType = fileInfo.mimeType;
    QString roomName = fileInfo.roomName;
    //QString base64Data = fileInfo.message;
    //QByteArray fileData = QByteArray::fromBase64(base64Data.toUtf8());

    QString fileId = generateUniqueFileId();
    QString filePath = QString("file_storage/%1").arg(fileId);

    //qDebug() << QString::number(fileInfo.messageLength) << fileSize;

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        fileStorage[fileId] = filePath;
        //file.write(fileData);
        file.write(*fileByte);
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

            Message msg;
            msg.SetMessageType(MessageType::file_data);
            msg.SetMimeType(fileId);
            msg.SetFileName(QFileInfo(filePath).fileName());
            msg.SetMessage(QString(fileData.toBase64()));
            sendMessage(*client, msg);

            qDebug() << QString("File downloaded by %1: %2")
                            .arg(clients[client], QFileInfo(filePath).fileName());
            ui->logTextEdit->appendPlainText(QString("File downloaded by %1: %2")
                                                 .arg(clients[client], QFileInfo(filePath).fileName()));
        } else {
            Message msg;
            msg.SetMessageType(MessageType::Error);
            msg.SetMessage(QString("Failed to read file data"));
            sendMessage(*client, msg);
        }
    } else {
//        QJsonObject response;
//        response["action"] = "error";
//        response["message"] = "File not found";
//        client->write(QJsonDocument(response).toJson());
        Message msg;
        msg.SetMessageType(MessageType::Error);
        msg.SetMessage(QString("File not found"));
        sendMessage(*client, msg);
    }
}

QString ServerManager::generateUniqueFileId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void ServerManager::sendMessageToClient(const QString& roomName, const QString& message, const QString& senderStr, QTcpSocket* client)
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

void ServerManager::sendPrevMessagesRoomToClient(const QString &roomName, QTcpSocket* client)
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
void ServerManager::sendMessage(QTcpSocket& client, Message& message)
{
    vector<QTcpSocket*> clients;
    clients.push_back(&client);
    vector<Message*> messages;
    messages.push_back(&message);
    sendMessage(clients, messages);
}

void ServerManager::sendMessage(vector<QTcpSocket*> &clients, vector<Message*> &messageList)
{
    QByteArray packet;
    for(auto msg: messageList){
        packet.append(msg->getByteArray());
    }

    for(auto client : clients){
        client->write(packet);
    }
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
