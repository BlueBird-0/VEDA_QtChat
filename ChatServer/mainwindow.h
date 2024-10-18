#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "servermanager.h"
#include "dbmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    DBManager dbManager;
    serverManager serverManager;
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionSetServer_triggered();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
