#ifndef MESSAGE_H
#define MESSAGE_H
#include <QByteArray>
class QString;

typedef enum {
    Text = 0,
    Login,
    LoginAck,
    File,
}MessageType;

class Message
{
public:
    char senderId[100];
    MessageType messageType;  //LOGIN, FILE, RTP_Connect 등등
    char message[BUFSIZ];
    Message();
    Message(QString id, MessageType messageType, QString message);
    Message(QByteArray data);

    void SetSenderId(QString str);
    void SetMessageType(MessageType str);
    void SetMessage(QString str);

    QByteArray getByteArray();
};

#endif // MESSAGE_H
