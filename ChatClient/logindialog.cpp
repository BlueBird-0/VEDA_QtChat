#include "logindialog.h"
#include "ui_logindialog.h"
#include "message.h"
#include "tcpclient.h"
#include "mainwindow.h"

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::on_btnLogin_clicked()
{
    bool loginSucess = false;

    //TODO : 로그인 기능 구현
    Message msg;
    msg.SetSenderId(ui->editID->toPlainText());
    msg.SetMessageType("Login");
    msg.SetMessage(ui->editPW->toPlainText()); //ID Check를 위해 PW를 함께 보냄.

    ((MainWindow*)qApp)->client.

    //TODO : 서버 응답 대기 및 로그인 확인 구현
    loginSucess = true;//TODO : 테스트용으로 모두다 강제로 loginSuccess.

    if(loginSucess){
        qDebug() << "login Success";
        this->accept();
    }else {
        //로그인 실패
    }
}

