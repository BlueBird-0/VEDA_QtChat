// servermanager.h
#ifndef SERVERMANAGER_H
#define SERVERMANAGER_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QSet>
using namespace std;
class MainWindow;
class Message;

namespace Ui {
class serverManager;
}

class serverManager : public QWidget
{
    Q_OBJECT

public:
    explicit serverManager(QWidget *parent = nullptr, MainWindow *mainWIndow = nullptr);
    ~serverManager();

private slots:
    void clientConnect();
    void clientDisconnect();
    void processMessage();
    void on_pushButton_clicked();

private:
    Ui::serverManager *ui;
    MainWindow* mainWindow;
    QTcpServer *tcpServer;
    QMap<QTcpSocket*, QString> clients;
    QMap<QString, QSet<QTcpSocket*>> rooms;

    void clearAllConnections();
    void Set_tcpServer();
    void updateClientList();
    void updateRoomList();
    void createRoom(QTcpSocket* client, const QString& roomName);
    void joinRoom(QTcpSocket* client, const QString& roomName);
    void leaveRoom(QTcpSocket* client, const QString& roomName);
    void sendMessageToRoom(const QString& roomName, const QString& message, QTcpSocket* sender);
    void handleLogin(QTcpSocket* client, const QString& username, const QString& password);
    //QMap<QTcpSocket*, QString> clients; // Map to store client sockets and their identifiers

};

#endif // SERVERMANAGER_H
