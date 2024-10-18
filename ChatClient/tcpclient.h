// tcpclient.h
#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QWidget>
#include <QTcpSocket>

namespace Ui {
class TcpClient;
}

class TcpClient : public QWidget
{
    Q_OBJECT

public:
    explicit TcpClient(QWidget *parent = nullptr);
    ~TcpClient();

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

private:
    Ui::TcpClient *ui;
    QTcpSocket *socket;
    QString currentRoom;
    QString username;
    bool isLoggedIn;

    void sendJson(const QJsonObject &jsonObj);
    void updateUIState();
};

#endif // TCPCLIENT_H
