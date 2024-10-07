#ifndef SERVERMANAGER_H
#define SERVERMANAGER_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
class QTextEdit;
class QPushButton;

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
    void on_pushButton_clicked();
    void clientConnect();
    void echoData();

private:
    QTcpServer *tcpServer = nullptr;
    void Set_tcpServer();

private:
    Ui::serverManager *ui;
};

#endif // SERVERMANAGER_H
