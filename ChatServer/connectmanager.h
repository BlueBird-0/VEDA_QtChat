#ifndef CONNECTMANAGER_H
#define CONNECTMANAGER_H

#include <QWidget>
#include <QTcpServer>
class QTextEdit;
class QPushButton;

class ConnectManager : public QWidget
{
    Q_OBJECT
public:
    explicit ConnectManager(QWidget *parent = 0);

private:
    QTcpServer *tcpServer;
    QTextEdit *ipEdit, *portEdit;
    QPushButton *setBtn;
};

#endif // CONNECTMANAGER_H
