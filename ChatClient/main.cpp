#include "mainwindow.h"

#include <QApplication>
#include "tcpclient.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    TcpClient client;
    client.show();
    // MainWindow w;
    // w.show();
    return a.exec();
}
