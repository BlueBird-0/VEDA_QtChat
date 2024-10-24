#ifndef MESSAGE_H
#define MESSAGE_H
#include <QByteArray>
#define SPLITKEY "@#!"
#define ENDKEY '$'
class QString;

typedef enum {
    Text = 0,
    Login,
    create_Room,
    join_Room,
    left_Room,
    send_Message,
    new_Message,
    uploadInit_file,
    upload_file,
    request_file,
    file_data,
    Error,
}MessageType;

#define MSGBUF 1024
class Message
{
public:
    char senderId[20];
    char roomName[40];
    MessageType messageType;  //LOGIN, FILE, RTP_Connect 등등
    char fileName[60];
    int fileSize;
    char mimeType[4];
    int messageLength;
    char message[MSGBUF];

    Message();
    Message(QByteArray data);

    void SetSenderId(QString str);
    void SetRoomName(QString str);
    void SetMessageType(MessageType str);
    void SetMessage(QString str);
    void SetFileName(QString str);
    void SetFileSize(int fileSize);
    void SetMessageLength(int msgLength);
    void SetMimeType(QString str);

    QByteArray getByteArray();
};

#endif // MESSAGE_H
