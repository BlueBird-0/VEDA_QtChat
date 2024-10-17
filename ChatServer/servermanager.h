#ifndef SERVERMANAGER_H
#define SERVERMANAGER_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
class MainWindow;

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
    void clearAllConnections();
    void Set_tcpServer();
    void clientConnect();
    void clientDisconnect();
    void echoData();
    void on_pushButton_clicked();
    void updateClientList();

private:
    Ui::serverManager *ui;
    MainWindow* mainWindow;
    QTcpServer *tcpServer;
    QMap<QTcpSocket*, QString> clients; // Map to store client sockets and their identifiers
};

#endif // SERVERMANAGER_H
