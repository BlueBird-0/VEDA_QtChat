// servermanager.h
#ifndef SERVERMANAGER_H
#define SERVERMANAGER_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QSet>
#include <QFile>

using namespace std;
class MainWindow;
class Message;

namespace Ui {
class ServerManager;
}

class DBManager;
class QSqlTableModel;
class QTableView;

class ServerManager : public QWidget
{
    Q_OBJECT

public:
    explicit ServerManager(QWidget *parent = nullptr);
    ~ServerManager();

private slots:
    void clientConnect();
    void clientDisconnect();
    void processMessage();
    void on_pushButton_clicked();

private:
    QMap<QString, QString> userCredentials;
    QMap<QString, QString> fileStorage; // Maps file IDs to file paths
    Ui::ServerManager *ui;
    DBManager* dbManager;

    QSqlTableModel* queryModel;
    QTableView *tableview;

    //Message Test table
    QSqlTableModel* messageQueryModel;
    QTableView *messageTableView;


    QTcpServer *tcpServer;
    QMap<QTcpSocket*, QString> clients;
    QMap<QString, QSet<QTcpSocket*>> rooms;

    void clearAllConnections();
    void Set_tcpServer();
    void updateClientList();
    void updateRoomList();
    bool authenticateUser(const QString& username, const QString& password);
    void handleLogin(QTcpSocket* client, const QString& username, const QString& password);
    void createRoom(QTcpSocket* client, const QString& roomName);
    void joinRoom(QTcpSocket* client, const QString& roomName);
    void leaveRoom(QTcpSocket* client, const QString& roomName);
    void sendMessage(QTcpSocket& clients, Message& messageList);
    void sendMessage(vector<QTcpSocket*> &clients, vector<Message*> &messageList);

    void sendMessageToRoom(const QString& roomName, const QString& message, QTcpSocket* sender);
    void handleFileUpload(QTcpSocket* sender, const QJsonObject& fileInfo);
    void sendFileToRoom(const QString& roomName, const QString& fileId, const QString& fileName, QTcpSocket* sender);
    void handleFileDownloadRequest(QTcpSocket* client, const QString& fileId);
    QString generateUniqueFileId();
    void sendMessageToClient(const QString& roomName, const QString& message, const QString& senderStr, QTcpSocket* client);
    void sendPrevMessagesRoomToClient(const QString &roomName, QTcpSocket* client);
    void loadMessageRoom(QString roomName);
};

#endif // SERVERMANAGER_H
