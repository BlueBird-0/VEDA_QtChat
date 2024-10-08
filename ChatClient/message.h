#ifndef MESSAGE_H
#define MESSAGE_H
#include <QByteArray>

class Message
{
public:
    char senderId[100];
    char messageType[100];  //LOGIN, FILE, RTP_Connect 등등
    char message[BUFSIZ];
    Message();
    Message(QByteArray data);

    QByteArray getByteArray();
};

#endif // MESSAGE_H
