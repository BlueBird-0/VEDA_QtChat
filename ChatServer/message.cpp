#include "message.h"
#include <QByteArray>
#include <QString>
#include <QStringList>

Message::Message() {}

Message::Message(QByteArray data) {
    // QByteArray를 QString로 변환하고, 구분자를 '\\'로 지정하여 분리
    QString dataString = QString::fromUtf8(data);
    QStringList parts = dataString.split("\\"); // '\\'를 기준으로 문자열을 분리

    // 분리된 값들을 각각 senderId, messageType, message에 할당
    if (parts.size() >= 3) {
        strncpy(senderId, parts[0].toUtf8().data(), sizeof(senderId) - 1);
        senderId[sizeof(senderId) - 1] = '\0';  // null-terminate

        strncpy(messageType, parts[1].toUtf8().data(), sizeof(messageType) - 1);
        messageType[sizeof(messageType) - 1] = '\0';  // null-terminate

        strncpy(message, parts[2].toUtf8().data(), sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';  // null-terminate
    }
}


QByteArray Message::getByteArray() {
    QByteArray data;
    data.append(senderId);
    data.append("\\");
    data.append(messageType);
    data.append("\\");
    data.append(message);
    data.append("\\");
    return data;
}
