#include "message.h"
#include <QByteArray>
#include <QString>
#include <QStringList>

Message::Message() {
    memset(this, 0, sizeof(Message));
}

Message::Message(QByteArray data) {
    // QByteArray를 QString로 변환하고, 구분자를 '\\'로 지정하여 분리
    QString dataString = QString::fromUtf8(data);
    QStringList parts = dataString.split("\\"); // '\\'를 기준으로 문자열을 분리

    // 분리된 값들을 각각 senderId, messageType, message에 할당
    if (parts.size() >= 6) {
        strncpy(senderId, parts[0].toUtf8().data(), sizeof(senderId) - 1);
        senderId[sizeof(senderId) - 1] = '\0';  // null-terminate

        strncpy(roomName, parts[1].toUtf8().data(), sizeof(roomName) - 1);
        roomName[sizeof(roomName) - 1] = '\0';  // null-terminate

        messageType = (MessageType)parts[2].toUtf8().toInt();

        strncpy(fileName, parts[3].toUtf8().data(), sizeof(fileName) - 1);
        fileName[sizeof(fileName) - 1] = '\0';  // null-terminate

        fileSize = parts[4].toUtf8().toInt();

        strncpy(mimeType, parts[5].toUtf8().data(), sizeof(mimeType) - 1);
        mimeType[sizeof(mimeType) - 1] = '\0';  // null-terminate

        messageLength = parts[6].toUtf8().toInt();

        strncpy(message, parts[7].toUtf8().data(), sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';  // null-terminate

    }
}

void Message::SetSenderId(QString str){
    memcpy(this->senderId, str.toStdString().c_str(), sizeof(this->senderId));
}
void Message::SetRoomName(QString str){
    memcpy(this->roomName, str.toStdString().c_str(), sizeof(this->roomName));
}
void Message::SetMessageType(MessageType messageType) {
    this->messageType = messageType;
}
void Message::SetMessage(QString str) {
    memcpy(this->message, str.toStdString().c_str(), sizeof(this->message));
}
void Message::SetFileName(QString str){
    memcpy(this->fileName, str.toStdString().c_str(), sizeof(this->fileName));
}
void Message::SetFileSize(int fileSize){
    this->fileSize = fileSize;
}
void Message::SetMessageLength(int msgLength) {
    this->messageLength = msgLength;
}
void Message::SetMimeType(QString str) {
    memcpy(this->mimeType, str.toStdString().c_str(), sizeof(this->mimeType));
}


QByteArray Message::getByteArray() {
    QByteArray data;
    data.append(senderId);
    data.append("\\");
    data.append(roomName);
    data.append("\\");
    data.append(QString::number(messageType).toStdString());
    data.append("\\");
    data.append(fileName);
    data.append("\\");
    data.append(QString::number(fileSize).toStdString());
    data.append("\\");
    data.append(mimeType);
    data.append("\\");
    data.append(QString::number(messageLength).toStdString());
    data.append("\\");
    data.append(message);
    return data;
}
