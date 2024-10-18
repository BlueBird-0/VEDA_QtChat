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
<<<<<<< HEAD
    Message(QString id, MessageType messageType, QString message);
=======
    Message(QString id, QString messageType, QString message);
>>>>>>> 61df6ebe64b8f652de18b686941f6f65bca7550a
    Message(QByteArray data);

    void SetSenderId(QString str);
    void SetMessageType(MessageType str);
    void SetMessage(QString str);

    QByteArray getByteArray();
};

#endif // MESSAGE_H
