#ifndef MESSAGE_H
#define MESSAGE_H
#include <QByteArray>
class QString;

typedef enum {
    Text = 0,
    Login,
    File,
    create_Room,
    join_Room,
    left_Room,
    send_Message,
    new_Message,
    Error,
}MessageType;

class Message
{
public:
    char senderId[100];
    char roomName[100];
    MessageType messageType;  //LOGIN, FILE, RTP_Connect 등등
    char message[BUFSIZ];
    Message();
    Message(QByteArray data);

    void SetSenderId(QString str);
    void SetRoomName(QString str);
    void SetMessageType(MessageType str);
    void SetMessage(QString str);

    QByteArray getByteArray();
};

#endif // MESSAGE_H
