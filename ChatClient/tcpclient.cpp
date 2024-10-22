// tcpclient.cpp
#include "tcpclient.h"
#include "ui_tcpclient.h"
#include <QMessageBox>
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

    connect(this, &TcpClient::sendMessage, this, &TcpClient::on_sendMessage);

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

    Message msg;
    msg.SetSenderId(username);
    msg.SetMessageType(MessageType::Login);
    msg.SetMessage(password);
    ui->messageEdit->clear();

    emit sendMessage(msg);  //서버에 msg 전송하는 이벤트 발생

}

void TcpClient::on_sendButton_clicked()
{
    if(isLoggedIn && !currentRoom.isEmpty()) {
        QString message = ui->messageEdit->text();
        Message msg;
        msg.SetSenderId(username);
        msg.SetRoomName(currentRoom);
        msg.SetMessageType(MessageType::send_Message);
        msg.SetMessage(message);
        ui->messageEdit->clear();

        emit sendMessage(msg);  //서버에 msg 전송하는 이벤트 발생
    }
}

void TcpClient::on_sendMessage(Message msg)
{
    if(socket->state() == QAbstractSocket::ConnectedState) {
        // server에 Message 객체 전송
        socket->write(msg.getByteArray());
    } else {
        QMessageBox::warning(this, "Warning", "Not connected to server");
    }
}

void TcpClient::on_createRoomButton_clicked()
{
    QString roomName = ui->roomNameEdit->text();
    if(!roomName.isEmpty()) {
        Message msg;
        msg.SetSenderId(username);
        msg.SetMessageType(MessageType::create_Room);
        msg.SetMessage(roomName);
        sendMessage(msg);
    }
}

void TcpClient::on_joinRoomButton_clicked()
{
    QString roomName = ui->roomNameEdit->text();
    if(!roomName.isEmpty()) {
        Message msg;
        msg.SetSenderId(username);
        msg.SetMessageType(MessageType::join_Room);
        msg.SetMessage(roomName);
        sendMessage(msg);
    }
}

void TcpClient::on_leaveRoomButton_clicked()
{
    if(!currentRoom.isEmpty()) {
        Message msg;
        msg.SetMessageType(MessageType::left_Room);
        msg.SetRoomName(currentRoom);
        sendMessage(msg);
    }
}

void TcpClient::onReadyRead()
{
    QByteArray byteArray = socket->readAll();
        vector<Message> messageList;
        recvMessage(byteArray, messageList);

        for(auto msg : messageList){
            if(msg.messageType == MessageType::Login)
            {
                if(msg.message == QString("Success"))
                {
                    isLoggedIn = true;
                    ui->chatDisplay->appendPlainText("Logged in successfully");
                    qDebug() << "login Success";
                } else {
                    // 로그인 실패
                    ui->chatDisplay->appendPlainText("Login failed: " + QString(msg.message));
                    qDebug() << "login Failed";
                }
                updateUIState();
            } else if (msg.messageType == MessageType::create_Room || msg.messageType == MessageType::join_Room) {
                currentRoom = QString(msg.message);
                ui->chatDisplay->appendPlainText(QString("Entered room: %1").arg(currentRoom));
                updateUIState();
            } else if (msg.messageType == MessageType::new_Message) {
                QString sender = msg.senderId;
                QString message = msg.message;
                ui->chatDisplay->appendPlainText(QString("%1: %2").arg(sender, message));
            } else if (msg.messageType == MessageType::Error) {
                QString errorMessage = msg.message;
                QMessageBox::warning(this, "Error", errorMessage);
            } else if (msg.messageType == MessageType::left_Room) {
                ui->chatDisplay->appendPlainText(QString("Left room: %1").arg(currentRoom));
                currentRoom.clear();
                updateUIState();
            }
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

void TcpClient::recvMessage(QByteArray &byteArray, vector<Message>& recvMsgList)
{
    // QByteArray를 QString로 변환하고, 구분자를 '\\'로 지정하여 분리
    QString dataString = QString::fromUtf8(byteArray);
    QStringList splits = dataString.split("&&"); // '\\'를 기준으로 문자열을 분리
    qDebug()<< "recvMsgList[" << splits.size() << "] :" << dataString;

    for(int i=0; i<splits.size()-1; i++)
    {
        QByteArray msgBytes = splits[i].toUtf8();

        recvMsgList.push_back(msgBytes);
    }
}
