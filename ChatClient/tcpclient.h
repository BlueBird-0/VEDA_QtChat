// tcpclient.h
#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QWidget>
#include <QTcpSocket>
#include <QFile>
#include <QFileDialog>
#include <QProgressDialog>
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
    QTcpSocket *socket;

private slots:
    void on_connectButton_clicked();
    void on_loginButton_clicked();
    void on_sendButton_clicked();
    void on_createRoomButton_clicked();
    void on_joinRoomButton_clicked();
    void on_leaveRoomButton_clicked();
    void on_sendFileButton_clicked();
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void handleFileDownloadRequest(const QUrl& fileId);
    void onFileDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onFileDownloadFinished();
    void on_sendMessage(Message msg); // Message 객체를 받는 새로운 슬롯
signals:
    void sendMessage(Message msg); // Message 객체를 전달하는 시그널
private:
    void updateUIState();
    void sendJson(const QJsonObject &jsonObj);
    void processFileDownload(const Message &msg);
    void appendMessage(const QString &sender, const QString &message, bool isFile = false);

    Ui::TcpClient *ui;
    QString username;
    QString currentRoom;
    bool isLoggedIn;
    QFile *currentDownloadFile;
    QProgressDialog *downloadProgress;
    qint64 currentFileSize;
    QString currentFileId;
    QMap<QString, QString> fileLinks;

    void recvMessage(QByteArray &byteArray, vector<Message>& recvMsgList);
};

#endif // TCPCLIENT_H
