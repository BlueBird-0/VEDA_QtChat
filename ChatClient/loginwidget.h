#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QWidget>

namespace Ui {
class LoginWidget;
}

class Message;
class LoginInfo{
public :
    static bool loginSuccess;   //로그인 상태
    static QString loginedId;   //로그인된 ID
    //static QString loginedCache; //로그인된 PW (cache?)
};

class LoginWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWidget(QWidget *parent = nullptr);
    ~LoginWidget();

private slots:
    void on_btnLogin_clicked();

signals:
    void loginRequested(const Message &msg);  // 로그인 요청 시그널
    void loginResponseReceived(bool);

private:
    QString loginedId;
    Ui::LoginWidget *ui;
};

#endif // LOGINWIDGET_H
