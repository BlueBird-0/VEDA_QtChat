// tcpclient.cpp
#include "tcpclient.h"
#include "ui_tcpclient.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QMimeDatabase>

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

    QJsonObject jsonObj;
    jsonObj["action"] = "login";
    jsonObj["username"] = username;
    jsonObj["password"] = password;
    sendJson(jsonObj);
}

void TcpClient::on_sendButton_clicked()
{
    if(isLoggedIn && !currentRoom.isEmpty()) {
        QString message = ui->messageEdit->text();
        if (!message.isEmpty()) {
            QJsonObject jsonObj;
            jsonObj["action"] = "send_message";
            jsonObj["room"] = currentRoom;
            jsonObj["message"] = message;
            sendJson(jsonObj);
            appendMessage(username, message); // Show own message immediately
            ui->messageEdit->clear();
        }
    }
}

void TcpClient::on_createRoomButton_clicked()
{
    QString roomName = ui->roomNameEdit->text();
    if(!roomName.isEmpty()) {
        QJsonObject jsonObj;
        jsonObj["action"] = "create_room";
        jsonObj["room"] = roomName;
        sendJson(jsonObj);
    }
}

void TcpClient::on_joinRoomButton_clicked()
{
    QString roomName = ui->roomNameEdit->text();
    if(!roomName.isEmpty()) {
        QJsonObject jsonObj;
        jsonObj["action"] = "join_room";
        jsonObj["room"] = roomName;
        sendJson(jsonObj);
    }
}

void TcpClient::on_leaveRoomButton_clicked()
{
    if(!currentRoom.isEmpty()) {
        QJsonObject jsonObj;
        jsonObj["action"] = "leave_room";
        jsonObj["room"] = currentRoom;
        sendJson(jsonObj);
        currentRoom.clear();
        updateUIState();
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

    // First, send file info to server
    QJsonObject jsonObj;
    jsonObj["action"] = "init_file_upload";
    jsonObj["filename"] = fileInfo.fileName();
    jsonObj["filesize"] = fileInfo.size();
    jsonObj["mimetype"] = mimeType;
    jsonObj["room"] = currentRoom;

    // Read file content
    QByteArray fileData = file.readAll();
    file.close();

    // // Send file content
    // QJsonObject fileObj;
    // fileObj["action"] = "upload_file";
    // fileObj["room"] = currentRoom;
    jsonObj["data"] = QString(fileData.toBase64());
    // sendJson(fileObj);
    sendJson(jsonObj);

    // // Show own file message immediately
    appendMessage(username, fileInfo.fileName(), true);
}

void TcpClient::onReadyRead()
{
    QByteArray data = socket->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
    QJsonObject jsonObj = jsonDoc.object();

    QString action = jsonObj["action"].toString();

    qDebug() << action << "\n";

    if (action == "login_response") {
        bool success = jsonObj["success"].toBool();
        if (success) {
            isLoggedIn = true;
            ui->chatDisplay->append("Logged in successfully");
        } else {
            ui->chatDisplay->append("Login failed: " + jsonObj["message"].toString());
        }
        updateUIState();
    } else if (action == "room_created" || action == "joined_room") {
        currentRoom = jsonObj["room"].toString();
        ui->chatDisplay->append(QString("Entered room: %1").arg(currentRoom));
        updateUIState();
    } else if (action == "left_room") {
        ui->chatDisplay->append(QString("Left room: %1").arg(currentRoom));
        currentRoom.clear();
        updateUIState();
    } else if (action == "new_message") {
        QString sender = jsonObj["sender"].toString();
        QString message = jsonObj["message"].toString();
        appendMessage(sender, message);
    } else if (action == "file_shared") {
        QString sender = jsonObj["sender"].toString();
        QString fileId = jsonObj["fileId"].toString();
        QString fileName = jsonObj["fileName"].toString();
        fileLinks[fileId] = fileName;
        appendMessage(sender, fileId, true);
    } else if (action == "file_data") {
        processFileDownload(jsonObj);
    } else if (action == "error") {
        QString errorMessage = jsonObj["message"].toString();
        QMessageBox::warning(this, "Error", errorMessage);
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

void TcpClient::sendJson(const QJsonObject &jsonObj)
{
    QJsonDocument jsonDoc(jsonObj);
    socket->write(jsonDoc.toJson());
}

void TcpClient::handleFileDownloadRequest(const QUrl& fileId)
{
    QString urlString = fileId.toString();
    QJsonObject jsonObj;
    jsonObj["action"] = "request_file";
    jsonObj["fileId"] = urlString;
    sendJson(jsonObj);

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

void TcpClient::processFileDownload(const QJsonObject &jsonObj)
{
    QString fileName = jsonObj["fileId"].toString();
    QString saveFilePath = QFileDialog::getSaveFileName(this, "Save File As", fileLinks.value(fileName, fileName));

    if (saveFilePath.isEmpty()) return;

    currentDownloadFile = new QFile(saveFilePath);
    if (!currentDownloadFile->open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Error", "Could not open file for writing");
        delete currentDownloadFile;
        currentDownloadFile = nullptr;
        return;
    }

    QByteArray fileData = QByteArray::fromBase64(jsonObj["data"].toString().toLatin1());
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
