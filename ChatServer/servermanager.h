#ifndef SERVERMANAGER_H
#define SERVERMANAGER_H

#include <QWidget>
#include <QTcpServer>
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

private:
    QTcpServer *tcpServer = nullptr;

private:
    Ui::serverManager *ui;
};

#endif // SERVERMANAGER_H
