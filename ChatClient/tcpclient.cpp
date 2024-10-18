#include "tcpclient.h"
#include "ui_tcpclient.h"
#include <QMessageBox>
#include "message.h"
#include "loginwidget.h"
using namespace std;

TcpClient::TcpClient(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TcpClient),
    socket(new QTcpSocket(this))
{
    ui->setupUi(this);

    connect(socket, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(socket, &QTcpSocket::connected, this, &TcpClient::onConnected);
    connect(socket, &QTcpSocket::disconnected, this, &TcpClient::onDisconnected);

    // sendMessage 시그널과 on_sendMessage 슬롯을 연결
    connect(this, &TcpClient::sendMessage, this, &TcpClient::on_sendMessage);

    ui->serverIP->setText("127.0.0.1");
    ui->serverPort->setText("54321");
}

TcpClient::~TcpClient()
{
    /* if socket connectec, disconnect */
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

    //login
    LoginWidget *login = new LoginWidget();
    login->show();
    connect(login, &LoginWidget::loginRequested, this, &TcpClient::sendMessage);

}

void TcpClient::on_sendButton_clicked()
{
    Message msg;
    msg.SetSenderId(LoginInfo::loginedId);
    msg.SetMessageType(MessageType::Login);
    msg.SetMessage(ui->messageEdit->text());
    ui->messageEdit->clear();

    emit sendMessage(msg);  //서버에 msg 전송하는 이벤트 발생
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

void TcpClient::onReadyRead()
{
    QByteArray byteArray = socket->readAll();
    vector<Message> messageList;
    recvMessage(byteArray, messageList);

    for(auto msg : messageList){
        ui->chatDisplay->appendPlainText(QString::fromUtf8(msg.senderId) + " : " + QString::fromUtf8(msg.message));
        if(msg.messageType == MessageType::LoginAck)
        {
            if(msg.message == QString("Success"))
            {
                LoginInfo::loginSuccess = true;
                LoginInfo::loginedId =msg.senderId;

                qDebug() << "login Success";
            } else {
                // 로그인 실패
                qDebug() << "login Failed";
            }
        }
    }
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

void TcpClient::onConnected()
{
    ui->connectButton->setText("Disconnect");
    ui->chatDisplay->appendPlainText("Connected to server");
}

void TcpClient::onDisconnected()
{
    ui->connectButton->setText("Connect");
    ui->chatDisplay->appendPlainText("Disconnected from server");
}
