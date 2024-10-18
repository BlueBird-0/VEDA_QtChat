#include "message.h"
#include <QByteArray>
#include <QString>
#include <QStringList>

Message::Message() {}

Message::Message(QString id, MessageType messageType, QString message){
    this->SetSenderId(id);
    this->SetMessageType(messageType);
    this->SetMessage(message);
}

Message::Message(QByteArray data) {
    // QByteArray를 QString로 변환하고, 구분자를 '\\'로 지정하여 분리
    QString dataString = QString::fromUtf8(data);
    QStringList parts = dataString.split("\\"); // '\\'를 기준으로 문자열을 분리

    // 분리된 값들을 각각 senderId, messageType, message에 할당
    if (parts.size() >= 3) {
        strncpy(senderId, parts[0].toUtf8().data(), sizeof(senderId) - 1);
        senderId[sizeof(senderId) - 1] = '\0';  // null-terminate

        //strncpy(messageType, parts[1].toUtf8().data(), sizeof(messageType));
        QString msgTypeStr = parts[1].toUtf8().data();
        messageType = (MessageType) (msgTypeStr.toInt());
        //messageType[sizeof(messageType) - 1] = '\0';  // null-terminate

        strncpy(message, parts[2].toUtf8().data(), sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';  // null-terminate
    }
}

void Message::SetSenderId(QString str){
    memcpy(this->senderId, str.toStdString().c_str(), sizeof(this->senderId));
}
void Message::SetMessageType(MessageType messageType) {
    this->messageType = messageType;
}
void Message::SetMessage(QString str) {
    memcpy(this->message, str.toStdString().c_str(), sizeof(this->message));
}


QByteArray Message::getByteArray() {
    QByteArray data;
    data.append(senderId);
    data.append("\\");
    data.append(QString::number(messageType));
    data.append("\\");
    data.append(message);
    data.append("\\");
    return data;
}
