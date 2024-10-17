#ifndef MESSAGE_H
#define MESSAGE_H
#include <QByteArray>
class QString;

class Message
{
public:
    char senderId[100];
    char messageType[100];  //LOGIN, FILE, RTP_Connect 등등
    char message[BUFSIZ];
    Message();
    Message(QString id, QString messageType, QString message);
    Message(QByteArray data);

    void SetSenderId(QString str);
    void SetMessageType(QString str);
    void SetMessage(QString str);

    QByteArray getByteArray();
};

#endif // MESSAGE_H
