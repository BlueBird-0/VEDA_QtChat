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

//    QJsonObject jsonObj;
//    jsonObj["action"] = "login";
//    jsonObj["username"] = username;
//    jsonObj["password"] = password;
//    sendJson(jsonObj);

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
//        QString message = ui->messageEdit->text();
//        QJsonObject jsonObj;
//        jsonObj["action"] = "send_message";
//        jsonObj["room"] = currentRoom;
//        jsonObj["message"] = message;
//        sendJson(jsonObj);
//        ui->messageEdit->clear();

        Message msg;
        //msg.SetSenderId(LoginInfo::loginedId);
       // msg.SetMessageType(MessageType::Login);
       // msg.SetMessage(ui->messageEdit->text());
       // ui->messageEdit->clear();

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
//    QByteArray data = socket->readAll();
//    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
//    QJsonObject jsonObj = jsonDoc.object();

//    QString action = jsonObj["action"].toString();
//    qDebug() << "onReadyRead(): action[" <<action <<"]";

//    if (action == "login_response") {
//        bool success = jsonObj["success"].toBool();
//        if (success) {
//            isLoggedIn = true;
//            ui->chatDisplay->appendPlainText("Logged in successfully");
//        } else {
//            ui->chatDisplay->appendPlainText("Login failed: " + jsonObj["message"].toString());
//        }
//        updateUIState();
//    } else if (action == "room_created" || action == "joined_room") {
//        currentRoom = jsonObj["room"].toString();
//        ui->chatDisplay->appendPlainText(QString("Entered room: %1").arg(currentRoom));
//        updateUIState();
//    } else if (action == "left_room") {
//        ui->chatDisplay->appendPlainText(QString("Left room: %1").arg(currentRoom));
//        currentRoom.clear();
//        updateUIState();
//    } else if (action == "new_message") {
//        QString sender = jsonObj["sender"].toString();
//        QString message = jsonObj["message"].toString();
//        ui->chatDisplay->appendPlainText(QString("%1: %2").arg(sender, message));
//    } else if (action == "error") {
//        QString errorMessage = jsonObj["message"].toString();
//        QMessageBox::warning(this, "Error", errorMessage);
//    }

    QByteArray byteArray = socket->readAll();
        vector<Message> messageList;
        recvMessage(byteArray, messageList);

        for(auto msg : messageList){
            ui->chatDisplay->appendPlainText(QString::fromUtf8(msg.senderId) + " : " + QString::fromUtf8(msg.message));
            if(msg.messageType == MessageType::Login)
            {
                if(msg.message == QString("Success"))
                {
                    //LoginInfo::loginSuccess = true;
                    //LoginInfo::loginedId =msg.senderId;

                    qDebug() << "login Success";
                } else {
                    // 로그인 실패
                    qDebug() << "login Failed";
                }
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

void TcpClient::sendJson(const QJsonObject &jsonObj)
{
    QJsonDocument jsonDoc(jsonObj);
    socket->write(jsonDoc.toJson());
}

void TcpClient::recvMessage(QByteArray &byteArray, vector<Message>& recvMsgList)
{
    // QByteArray를 QString로 변환하고, 구분자를 '\\'로 지정하여 분리
    QString dataString = QString::fromUtf8(byteArray);
    QStringList splits = dataString.split("\\"); // '\\'를 기준으로 문자열을 분리
    qDebug()<< "recvMsgList[" << splits.size() << "] :" << dataString;
    Message msg;

    for(int i=0; i<splits.size()-1; i+=3)
    {
        // 분리된 값들을 각각 senderId, messageType, message에 할당
        strncpy(msg.senderId, splits[i+0].toUtf8().data(), sizeof(msg.senderId) - 1);
        msg.senderId[sizeof(msg.senderId) - 1] = '\0';  // null-terminate

        QString msgTypeStr = splits[i+1].toUtf8().data();
        msg.messageType = (MessageType)msgTypeStr.toInt();


        strncpy(msg.message, splits[i+2].toUtf8().data(), sizeof(msg.message) - 1);
        msg.message[sizeof(msg.message) - 1] = '\0';  // null-terminate
        recvMsgList.push_back(msg);
    }
}
