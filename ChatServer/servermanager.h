// servermanager.h
#ifndef SERVERMANAGER_H
#define SERVERMANAGER_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QSet>

namespace Ui {
class serverManager;
}

class serverManager : public QWidget
{
    Q_OBJECT

public:
    explicit serverManager(QWidget *parent = nullptr);
    ~serverManager();

private slots:
    void clientConnect();
    void clientDisconnect();
    void processMessage();
    void on_pushButton_clicked();

private:
    Ui::serverManager *ui;
    QTcpServer *tcpServer;
    QMap<QTcpSocket*, QString> clients;
    QMap<QString, QSet<QTcpSocket*>> rooms;
    QMap<QString, QString> userCredentials;  // username -> password

    void clearAllConnections();
    void Set_tcpServer();
    void updateClientList();
    void updateRoomList();
    void createRoom(QTcpSocket* client, const QString& roomName);
    void joinRoom(QTcpSocket* client, const QString& roomName);
    void leaveRoom(QTcpSocket* client, const QString& roomName);
    void sendMessageToRoom(const QString& roomName, const QString& message, QTcpSocket* sender);
    bool authenticateUser(const QString& username, const QString& password);
    void handleLogin(QTcpSocket* client, const QString& username, const QString& password);
};

#endif // SERVERMANAGER_H
