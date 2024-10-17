#include "servermanager.h"
#include "ui_servermanager.h"
#include <QMessageBox>
#include <message.h>
#include <mainwindow.h>
using namespace std;

serverManager::serverManager(QWidget *parent, MainWindow* mainWindow)
    : QWidget(parent)
    , mainWindow(mainWindow)
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
    /*disconnect all the client connections */
    for (QTcpSocket *clientSocket : clients.keys()) {
        clientSocket->disconnectFromHost();
        clientSocket->deleteLater();
        updateClientList();
    }
    /* clear the client map*/
    clients.clear();
}

void serverManager::Set_tcpServer()
{
    delete tcpServer;
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &serverManager::clientConnect);


    QHostAddress ipAddr = QHostAddress::AnyIPv4;  // 모든 IP(v4)에서 리슨하도록 설정
    quint16 port = ui->portEdit->toPlainText().toUShort();

    if(!tcpServer->listen(ipAddr, port)) {
        QMessageBox::critical(this, tr("Echo Server"),
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
    connect(clientConnection, &QTcpSocket::readyRead, this, &serverManager::echoData);
    
    // Assign a temporary identifier (you can replace this with a login system later
    QString clientId = QString("Client_%1 (%2)")
                           .arg(clientConnection->peerPort()) // 정수값 그대로 사용 가능
                           .arg(clientConnection->peerAddress().toString()); // QHostAddress를 문자열로 변환
    clients[clientConnection] = clientId;
    
    qDebug() << "New connection established:" << clientId;
    updateClientList();
}

void serverManager::clientDisconnect()
{
    QTcpSocket *clientConnection = qobject_cast<QTcpSocket*>(sender());
    if (clientConnection) {
        QString clientId = clients.take(clientConnection);
        qDebug() << "Client disconnected:" << clientId;
        clientConnection->deleteLater();
        updateClientList();
    }
}


void serverManager::echoData()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket)
        return;

    QByteArray data = clientSocket->readAll();
    QString clientId = clients[clientSocket];

    Message recvMsg(data);
    qDebug() <<"["<< recvMsg.senderId<<"]"<<recvMsg.messageType<<"-"<<recvMsg.message;

    if( recvMsg.messageType == QString("Login")){
        bool loginSuccess = mainWindow->dbManager.checkLogin(recvMsg.senderId, recvMsg.message);
        Message ackMsg(recvMsg.senderId, "LoginAck", loginSuccess? "Success": "Fail");
        sendMessage(*clientSocket, ackMsg);
    }
    
    // Log the received message
    qDebug() << "Received from" << clientId << ":" << QString::fromUtf8(data);
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
    // Update UI with connected clients (you need to add a QListWidget to your UI for this)
    ui->clientListWidget->clear();
    for (const QString &clientId : clients.values()) {
        ui->clientListWidget->addItem(clientId);
    }
}
