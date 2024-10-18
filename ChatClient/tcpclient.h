// tcpclient.h
#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QWidget>
#include <QTcpSocket>
#include "message.h"
using namespace std;
class QByteArray;

namespace Ui {
class TcpClient;
}

class TcpClient : public QWidget
{
    Q_OBJECT

public:
    explicit TcpClient(QWidget *parent = nullptr);
    ~TcpClient();
    QTcpSocket *socket = nullptr;

private slots:
    void on_connectButton_clicked();
    void on_loginButton_clicked();
    void on_sendButton_clicked();
    void on_createRoomButton_clicked();
    void on_joinRoomButton_clicked();
    void on_leaveRoomButton_clicked();
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    //void on_sendMessage(Message msg); // Message 객체를 받는 새로운 슬롯

signals:
    void sendMessage(Message msg); // Message 객체를 전달하는 시그널

private:
    Ui::TcpClient *ui;
    QString currentRoom;
    QString username;
    bool isLoggedIn;

    void sendJson(const QJsonObject &jsonObj);
    void updateUIState();
    void recvMessage(QByteArray &byteArray, vector<Message>& recvMsgList);
};

#endif // TCPCLIENT_H
