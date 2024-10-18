#include <QApplication>
#include "loginwidget.h"
#include "ui_loginwidget.h"
#include "message.h"
#include "tcpclient.h"
#include "mainwindow.h"
#include <QProgressDialog>
#include <QTimer>
#include <QMessageBox>


//static 변수 초기화
QString LoginInfo::loginedId = "";
bool LoginInfo::loginSuccess = false;

LoginWidget::LoginWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoginWidget)
{
    ui->setupUi(this);
}

LoginWidget::~LoginWidget()
{
    delete ui;
}

void LoginWidget::on_btnLogin_clicked()
{
    Message msg;
    msg.SetSenderId(ui->editID->toPlainText());
    msg.SetMessageType(MessageType::Login);
    msg.SetMessage(ui->editPW->toPlainText());

    emit loginRequested(msg);

    // TODO: 서버 응답 대기 및 로그인 확인 구현
    // QEventLoop와 QTimer를 사용하여 5초 대기
    QEventLoop loop;
    QTimer timer;

    // 타이머 설정 (5초 후 타임아웃)
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, [&]() {
        loop.quit();  // 타임아웃 시 이벤트 루프 종료
        QMessageBox::warning(this, "Timeout", "Login timed out.");
    });

    // 타이머 시작 (5초)
    timer.start(5000);

    // loginSuccess가 true가 될 때까지 반복 확인
    QTimer checkTimer;
    checkTimer.setInterval(100);  // 100ms 간격으로 확인
    connect(&checkTimer, &QTimer::timeout, [&]() {
        if (LoginInfo::loginSuccess) {
            loop.quit();  // 로그인 성공 시 이벤트 루프 종료
        }
    });
    checkTimer.start();

    // 이벤트 루프 실행, loginSuccess가 true가 되거나 타임아웃이 발생할 때까지 대기
    loop.exec();

    // 대기 종료 후 로그인 성공 여부 처리
    if (LoginInfo::loginSuccess) {
        QMessageBox::information(this, "Login Success", "You are now logged in!");
    } else {
        QMessageBox::critical(this, "Login Failed", "Invalid credentials or timeout.");
    }
}

