#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QWidget>
#include <QTcpSocket>

namespace Ui {
class TcpClient;
}

class TcpClient : public QWidget {

    Q_OBJECT

public:
    explicit TcpClient(QWidget *parent = nullptr);
    ~TcpClient();

private slots:
    void on_connectButton_clicked();
    void on_sendButton_clicked();
    void onReadyRead();
    void onConnected();
    void onDisconnected();

private:
    Ui::TcpClient *ui;
    QTcpSocket *socket;
};

#endif // TCPCLIENT_H

