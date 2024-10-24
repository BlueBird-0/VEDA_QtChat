// tcpclient.cpp
#include "tcpclient.h"
#include "ui_tcpclient.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QMimeDatabase>
#include <QTextBrowser>
#include <QTimer>
#include <QMutex>
using namespace std;

TcpClient::TcpClient(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TcpClient),
    socket(new QTcpSocket(this)),
    isLoggedIn(false),
    currentDownloadFile(nullptr),
    downloadProgress(nullptr)
{
    ui->setupUi(this);

    connect(socket, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(socket, &QTcpSocket::connected, this, &TcpClient::onConnected);
    connect(socket, &QTcpSocket::disconnected, this, &TcpClient::onDisconnected);
    connect(ui->chatDisplay, &QTextBrowser::anchorClicked, this, &TcpClient::handleFileDownloadRequest);

    connect(this, &TcpClient::sendMessage, this, &TcpClient::on_sendMessage);

    ui->serverIP->setText("127.0.0.1");
    ui->serverPort->setText("5432");
    ui->chatDisplay->setOpenExternalLinks(false);
    ui->chatDisplay->setOpenLinks(false);
    updateUIState();
}

TcpClient::~TcpClient()
{
    delete currentDownloadFile;
    delete downloadProgress;
    if(socket->state() == QAbstractSocket::ConnectedState) {
        socket->disconnectFromHost();
    }
    delete ui;
}

void TcpClient::updateUIState()
{
    bool connected = socket->state() == QAbstractSocket::ConnectedState;
    ui->serverIP->setEnabled(!connected);
    ui->serverPort->setEnabled(!connected);
    ui->connectButton->setText(connected ? "Disconnect" : "Connect");

    ui->usernameEdit->setEnabled(connected && !isLoggedIn);
    ui->passwordEdit->setEnabled(connected && !isLoggedIn);
    ui->loginButton->setEnabled(connected && !isLoggedIn);

    ui->roomNameEdit->setEnabled(isLoggedIn);
    ui->createRoomButton->setEnabled(isLoggedIn);
    ui->joinRoomButton->setEnabled(isLoggedIn);
    ui->leaveRoomButton->setEnabled(isLoggedIn && !currentRoom.isEmpty());
    ui->messageEdit->setEnabled(isLoggedIn && !currentRoom.isEmpty());
    ui->sendButton->setEnabled(isLoggedIn && !currentRoom.isEmpty());
    ui->sendFileButton->setEnabled(isLoggedIn && !currentRoom.isEmpty());
}

void TcpClient::on_connectButton_clicked()
{
    if(socket->state() == QAbstractSocket::UnconnectedState) {
        QString ip = ui->serverIP->text();
        quint16 port = ui->serverPort->text().toUShort();
        socket->connectToHost(ip, port);
    } else {
        socket->disconnectFromHost();
    }
}

void TcpClient::on_loginButton_clicked()
{
    username = ui->usernameEdit->text();
    QString password = ui->passwordEdit->text();

    Message msg;
    msg.SetSenderId(username);
    msg.SetMessageType(MessageType::Login);
    msg.SetMessage(password);
    ui->messageEdit->clear();

    emit sendMessage(msg);  //서버에 msg 전송하는 이벤트 발생

}

void TcpClient::on_sendButton_clicked()
{
    if(isLoggedIn && !currentRoom.isEmpty()) {
        QString message = ui->messageEdit->text();
        Message msg;
        msg.SetSenderId(username);
        msg.SetRoomName(currentRoom);
        msg.SetMessageType(MessageType::send_Message);
        msg.SetMessage(message);
        ui->messageEdit->clear();

        emit sendMessage(msg);  //서버에 msg 전송하는 이벤트 발생
    }
}

QMutex tcpMutex;
void TcpClient::on_sendMessage(Message msg)
{
    if(socket->state() == QAbstractSocket::ConnectedState) {
        // server에 Message 객체 전송
        QByteArray msgBytes = msg.getByteArray();
        tcpMutex.lock();
        //qDebug() << "on_sendMessage :"<<msgBytes.size();
        socket->write(msgBytes);
        tcpMutex.unlock();
//        QEventLoop loop;
//        QTimer::singleShot(10, &loop, SLOT(quit())); // 2-second delay
  //      loop.exec();  // Process events until quit() is called

    } else {
        QMessageBox::warning(this, "Warning", "Not connected to server");
    }
}

void TcpClient::on_createRoomButton_clicked()
{
    QString roomName = ui->roomNameEdit->text();
    if(!roomName.isEmpty()) {
        Message msg;
        msg.SetSenderId(username);
        msg.SetMessageType(MessageType::create_Room);
        msg.SetMessage(roomName);
        emit sendMessage(msg);
    }
}

void TcpClient::on_joinRoomButton_clicked()
{
    QString roomName = ui->roomNameEdit->text();
    if(!roomName.isEmpty()) {
        Message msg;
        msg.SetSenderId(username);
        msg.SetMessageType(MessageType::join_Room);
        msg.SetMessage(roomName);
        sendMessage(msg);
    }
}

void TcpClient::on_leaveRoomButton_clicked()
{
    if(!currentRoom.isEmpty()) {
        Message msg;
        msg.SetMessageType(MessageType::left_Room);
        msg.SetRoomName(currentRoom);
        sendMessage(msg);
    }
}

void TcpClient::on_sendFileButton_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Select File to Send");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Could not open file for reading");
        return;
    }

    QFileInfo fileInfo(file);
    QMimeDatabase db;
    QString mimeType = db.mimeTypeForFile(fileInfo).name();

    // Read file content
    QByteArray fileData = file.readAll();


    qDebug() << "File Size = " <<fileInfo.size();
    int fileSize = fileInfo.size();
    int sendSize = 0;

    bool uploadInit = false;
    while(sendSize != fileSize){
        int remainSize = fileSize-sendSize;
        QByteArray chunk = fileData.mid(sendSize, min(MSGBUF-1, remainSize));// BUFSIZ 크기만큼 자름
        sendSize += min(MSGBUF-1, fileSize-sendSize);

        Message msg;
        if(!uploadInit){
            msg.SetMessageType(MessageType::uploadInit_file);
            uploadInit = true;
        }else{
            msg.SetMessageType(MessageType::upload_file);
        }
        msg.SetFileName(fileInfo.fileName());
        msg.SetFileSize((int)fileInfo.size());
        msg.SetMimeType(mimeType);
        msg.SetRoomName(currentRoom);

        msg.SetMessageLength(chunk.length());
        msg.SetMessage(chunk);

        qDebug() << "chunk Size = " <<chunk.length();
        emit sendMessage(msg);   //TODO 대용량 파일 전송 확인해보기
    }
    file.close();

    // // Show own file message immediately
    appendMessage(username, fileInfo.fileName(), true);
}

void TcpClient::onReadyRead()
{
//    } else if (action == "file_shared") {
//        QString sender = jsonObj["sender"].toString();
//        QString fileId = jsonObj["fileId"].toString();
//        QString fileName = jsonObj["fileName"].toString();
//        fileLinks[fileId] = fileName;
//        appendMessage(sender, fileId, true);
//    } else if (action == "file_data") {
//        processFileDownload(jsonObj);
//    } else if (action == "error") {
//        QString errorMessage = jsonObj["message"].toString();
//        QMessageBox::warning(this, "Error", errorMessage);
//    }
    QByteArray byteArray = socket->readAll();
    qDebug() <<"message:" << byteArray;

    QList<QByteArray> byteList =byteArray.split(ENDKEY);
    while(!byteList.empty()){
        QByteArray *msgBytes = &byteList.front();
        byteList.pop_front();
        if(msgBytes->size() == 0){
            break;
        }
        Message msg(*msgBytes);

        if(msg.messageType == MessageType::Login)
        {
            if(msg.message == QString("Success"))
            {
                isLoggedIn = true;
                appendMessage("System", "Logged in successfully", false);
                qDebug() << "login Success";
            } else {
                // 로그인 실패
                appendMessage("System", "Login failed: " + QString(msg.message), false);
                qDebug() << "login Failed";
            }
            updateUIState();
        } else if (msg.messageType == MessageType::create_Room || msg.messageType == MessageType::join_Room) {
            currentRoom = QString(msg.message);
            appendMessage("System", QString("Entered room: %1").arg(currentRoom), false);
            updateUIState();
        } else if (msg.messageType == MessageType::new_Message) {
            QString sender = msg.senderId;
            QString message = msg.message;
            appendMessage("Ssytem", QString("%1: %2").arg(sender, message),  false);
        } else if (msg.messageType == MessageType::Error) {
            QString errorMessage = msg.message;
            QMessageBox::warning(this, "Error", errorMessage);
        } else if (msg.messageType == MessageType::left_Room) {
            appendMessage("System", QString("Left room: %1").arg(currentRoom), false);
            currentRoom.clear();
            updateUIState();
        } else if (msg.messageType == MessageType::file_data) {
            processFileDownload(msg);
        }
    }
}

void TcpClient::onConnected()
{
    ui->chatDisplay->append("Connected to server");
    updateUIState();
}

void TcpClient::onDisconnected()
{
    ui->chatDisplay->append("Disconnected from server");
    isLoggedIn = false;
    currentRoom.clear();
    updateUIState();
}

void TcpClient::recvMessage(QByteArray &byteArray, vector<Message>& recvMsgList)
{
    // QByteArray를 QString로 변환하고, 구분자를 '\\'로 지정하여 분리
    QString dataString = QString::fromUtf8(byteArray);
    //QStringList splits = dataString.split("&");   //TODO 검토후 지울 것.
    //qDebug()<< "recvMsgList[" << splits.size() << "] :" << dataString;


    //for(int i=0; i<splits.size()-1; i++)
    {
       // QByteArray msgBytes = splits[i].toUtf8();

        //recvMsgList.push_back(msgBytes);
    }
}

void TcpClient::handleFileDownloadRequest(const QUrl& fileId)
{
    //QString urlString = fileId.toString();
    //QJsonObject jsonObj;
    //jsonObj["action"] = "request_file";
    //jsonObj["fileId"] = urlString;
    //sendJson(jsonObj);    //TODO : 확인

    Message msg;
    QString urlString = fileId.toString();
    msg.SetMessage(urlString);
    msg.SetMessageType(MessageType::request_file);
    sendMessage(msg);

    // downloadProgress = new QProgressDialog("Downloading file...", "Cancel", 0, 100, this);
    // downloadProgress->setWindowModality(Qt::WindowModal);
    // connect(downloadProgress, &QProgressDialog::canceled, this, [this]() {
    //     if (currentDownloadFile) {
    //         currentDownloadFile->close();
    //         currentDownloadFile->remove();
    //         delete currentDownloadFile;
    //         currentDownloadFile = nullptr;
    //     }
    // });
}

void TcpClient::processFileDownload(const Message &msg)
{
    //QString fileName = jsonObj["fileId"].toString();
    QString fileName = msg.mimeType;
    QString saveFilePath = QFileDialog::getSaveFileName(this, "Save File As", fileLinks.value(fileName, fileName));

    if (saveFilePath.isEmpty()) return;

    currentDownloadFile = new QFile(saveFilePath);
    if (!currentDownloadFile->open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Error", "Could not open file for writing");
        delete currentDownloadFile;
        currentDownloadFile = nullptr;
        return;
    }

    QByteArray fileData = QByteArray::fromBase64(QString(msg.message).toLatin1());
    currentDownloadFile->write(fileData);
    onFileDownloadFinished();
}

void TcpClient::onFileDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (downloadProgress && bytesTotal > 0) {
        int progress = (bytesReceived * 100) / bytesTotal;
        downloadProgress->setValue(progress);
    }
}

void TcpClient::onFileDownloadFinished()
{
    if (currentDownloadFile) {
        currentDownloadFile->close();
        delete currentDownloadFile;
        currentDownloadFile = nullptr;
    }
    if (downloadProgress) {
        downloadProgress->close();
        delete downloadProgress;
        downloadProgress = nullptr;
    }
    QMessageBox::information(this, "Download Complete", "File has been downloaded successfully.");
}

void TcpClient::appendMessage(const QString &sender, const QString &message, bool isFile)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString formattedMessage;

    if (isFile) {
        // For files, create a clickable link
        formattedMessage = QString("[%1] %2 shared a file: <a href='%3'>%4</a>")
                               .arg(timestamp, sender, message, fileLinks.value(message, message));
    } else {
        formattedMessage = QString("[%1] %2: %3")
        .arg(timestamp, sender, message);
    }

    ui->chatDisplay->append(formattedMessage);
}
