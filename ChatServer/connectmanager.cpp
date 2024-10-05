#include "connectmanager.h"
#include <QMessageBox>
#include <QTextEdit>
#include <QPushButton>
#include <QHBoxLayout>

ConnectManager::ConnectManager(QWidget *parent)
    : QWidget(parent)
{
    ipEdit = new QTextEdit(this);
    portEdit = new QTextEdit(this);
    setBtn = new QPushButton(this);

    QHBoxLayout *hBox = new QHBoxLayout();
    hBox->addWidget(ipEdit);
    hBox->addWidget(portEdit);
    hBox->addWidget(setBtn);
    setLayout(hBox);

    tcpServer = new QTcpServer(this);
    connect(tcpServer, SIGNAL(newConnection()), nullptr);
    if(!tcpServer->listen()){
        QMessageBox::critical(this, tr("Echo Server"), \
            tr("Unable to start the server: %1.") \
            .arg(tcpServer->errorString()));
        close();
        return;
    }

}
