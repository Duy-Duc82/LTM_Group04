#include "NetworkClient.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonValue>

// Command definitions (matching server)
#define CMD_REQ_REGISTER    0x0101
#define CMD_RES_REGISTER    0x0102
#define CMD_REQ_LOGIN       0x0103
#define CMD_RES_LOGIN       0x0104
#define CMD_REQ_LOGOUT      0x0106
#define CMD_RES_LOGOUT      0x0107

// QuickMode commands
#define CMD_REQ_START_QUICKMODE    0x0500
#define CMD_NOTIFY_GAME_START      0x0501
#define CMD_NOTIFY_QUESTION        0x0502
#define CMD_REQ_SUBMIT_ANSWER      0x0503
#define CMD_RES_SUBMIT_ANSWER      0x0504
#define CMD_NOTIFY_ANSWER_RESULT   0x0505
#define CMD_REQ_USE_LIFELINE       0x0506
#define CMD_RES_USE_LIFELINE       0x0507
#define CMD_NOTIFY_GAME_OVER       0x0509
#define CMD_REQ_GET_QUESTION       0x050A

// Room commands
#define CMD_REQ_CREATE_ROOM        0x0401
#define CMD_RES_CREATE_ROOM        0x0402
#define CMD_REQ_JOIN_ROOM          0x0403
#define CMD_RES_JOIN_ROOM          0x0404
#define CMD_NOTIFY_ROOM_UPDATE     0x0405
#define CMD_REQ_LEAVE_ROOM         0x040B
#define CMD_RES_LEAVE_ROOM         0x040C
#define CMD_REQ_START_GAME         0x040D
#define CMD_RES_START_GAME         0x040E

// 1vN commands
#define CMD_NOTIFY_GAME_START_1VN  0x0601
#define CMD_NOTIFY_QUESTION_1VN    0x0602
#define CMD_REQ_SUBMIT_ANSWER_1VN  0x0603
#define CMD_RES_SUBMIT_ANSWER_1VN  0x0604
#define CMD_NOTIFY_ELIMINATION     0x0605
#define CMD_NOTIFY_GAME_OVER_1VN   0x0606

// Friends commands
#define CMD_REQ_ADD_FRIEND         0x0201
#define CMD_RES_ADD_FRIEND         0x0202
#define CMD_NOTIFY_FRIEND_REQ      0x0203
#define CMD_REQ_LIST_FRIENDS       0x0206
#define CMD_RES_LIST_FRIENDS       0x0207
#define CMD_NOTIFY_FRIEND_STATUS   0x0208
#define CMD_REQ_SEARCH_USER        0x0209
#define CMD_RES_SEARCH_USER        0x020A
#define CMD_REQ_GET_FRIEND_INFO    0x020B
#define CMD_RES_GET_FRIEND_INFO    0x020C
#define CMD_REQ_GET_PENDING_REQ    0x020D
#define CMD_RES_GET_PENDING_REQ    0x020E
#define CMD_REQ_RESPOND_FRIEND     0x0204
#define CMD_RES_RESPOND_FRIEND     0x0205
#define CMD_REQ_REMOVE_FRIEND       0x020F
#define CMD_RES_REMOVE_FRIEND       0x0210
#define CMD_NOTIFY_FRIEND_REQ      0x0203

// Chat commands
#define CMD_REQ_SEND_DM            0x0301
#define CMD_RES_SEND_DM            0x0302
#define CMD_NOTIFY_DM              0x0303
#define CMD_REQ_FETCH_OFFLINE      0x0307
#define CMD_RES_FETCH_OFFLINE      0x0308

// Stats commands
#define CMD_REQ_GET_PROFILE        0x0701
#define CMD_RES_GET_PROFILE        0x0702
#define CMD_REQ_LEADERBOARD        0x0703
#define CMD_RES_LEADERBOARD        0x0704
#define CMD_REQ_MATCH_HISTORY      0x0705
#define CMD_RES_MATCH_HISTORY      0x0706
#define CMD_REQ_UPDATE_AVATAR      0x0707
#define CMD_RES_UPDATE_AVATAR      0x0708

NetworkClient::NetworkClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
    , m_loggedIn(false)
    , m_userId(0)
    , m_lastQuestionSessionId(0)
    , m_lastQuestionRound(0)
    , m_lastQuestionId(0)
{
    connect(m_socket, &QTcpSocket::readyRead, this, &NetworkClient::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &NetworkClient::onSocketError);
    connect(m_socket, &QTcpSocket::stateChanged, this, &NetworkClient::onSocketStateChanged);
}

NetworkClient::~NetworkClient()
{
    disconnectFromServer();
}

bool NetworkClient::connectToServer(const QString &host, quint16 port)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        qDebug() << "Already connected to server";
        emit errorOccurred("Đã kết nối đến server");
        return false;
    }

    qDebug() << "Connecting to server:" << host << ":" << port;
    m_socket->connectToHost(host, port);
    
    // Wait for connection (with timeout)
    if (!m_socket->waitForConnected(5000)) {  // Increased timeout to 5 seconds
        QString errorMsg = m_socket->errorString();
        qDebug() << "Connection failed:" << errorMsg;
        qDebug() << "Socket state:" << m_socket->state();
        qDebug() << "Socket error:" << m_socket->error();
        emit errorOccurred("Không thể kết nối: " + errorMsg);
        return false;
    }
    
    qDebug() << "Connected successfully to" << host << ":" << port;
    qDebug() << "Socket state:" << m_socket->state();
    // Signal will be emitted by onSocketStateChanged
    return true;
}

void NetworkClient::disconnectFromServer()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
    m_loggedIn = false;
    m_token.clear();
    m_buffer.clear();
}

bool NetworkClient::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

bool NetworkClient::isLoggedIn() const
{
    return m_loggedIn;
}

QString NetworkClient::getToken() const
{
    return m_token;
}

QByteArray NetworkClient::createPacketHeader(quint16 cmd, quint16 user_id, quint32 length)
{
    QByteArray header;
    QDataStream stream(&header, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);  // Network byte order
    stream << cmd << user_id << length;
    return header;
}

void NetworkClient::sendPacket(quint16 cmd, quint16 user_id, const QByteArray &json)
{
    if (!isConnected()) {
        emit errorOccurred("Chưa kết nối đến server");
        return;
    }

    QByteArray header = createPacketHeader(cmd, user_id, json.size());
    m_socket->write(header);
    m_socket->write(json);
    m_socket->flush();

    qDebug() << "Sent packet: cmd=" << QString::number(cmd, 16) 
             << "user_id=" << user_id << "length=" << json.size();

    // Use async signal/slot mechanism only - no blocking wait
    // Response will be handled by onReadyRead() when data arrives
    // Check if data is already available (non-blocking)
    if (m_socket->bytesAvailable() > 0) {
        qDebug() << "Data already available, reading response...";
        onReadyRead();
    }
}

void NetworkClient::sendRegister(const QString &username, const QString &password)
{
    QJsonObject obj;
    obj["username"] = username;
    obj["password"] = password;

    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);

    sendPacket(CMD_REQ_REGISTER, 0, json);
}

void NetworkClient::sendLogin(const QString &username, const QString &password)
{
    if (!isConnected()) {
        qDebug() << "Cannot send login: not connected to server";
        emit errorOccurred("Chưa kết nối đến server. Vui lòng kết nối trước.");
        return;
    }

    // No password validation - send as-is
    QJsonObject obj;
    obj["username"] = username;
    obj["password"] = password;

    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);

    qDebug() << "=== SENDING LOGIN REQUEST ===";
    qDebug() << "Username:" << username;
    qDebug() << "Password:" << password;
    qDebug() << "Password length:" << password.length();
    qDebug() << "Login JSON:" << json;
    qDebug() << "============================";
    sendPacket(CMD_REQ_LOGIN, 0, json);
}

void NetworkClient::sendLogout()
{
    QJsonObject obj;  // Empty object
    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);

    quint16 user_id = m_loggedIn ? m_userId : 0;
    sendPacket(CMD_REQ_LOGOUT, user_id, json);
}

quint16 NetworkClient::getUserId() const
{
    return m_userId;
}

void NetworkClient::sendStartQuickMode()
{
    QJsonObject obj;  // Empty object
    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);

    sendPacket(CMD_REQ_START_QUICKMODE, m_userId, json);
}

void NetworkClient::sendGetQuestion(qint64 sessionId, int round)
{
    qDebug() << "sendGetQuestion: sessionId=" << sessionId << "round=" << round;
    QJsonObject obj;
    obj["session_id"] = sessionId;
    obj["round"] = round;

    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);
    
    qDebug() << "Sending CMD_REQ_GET_QUESTION with JSON:" << json;

    sendPacket(CMD_REQ_GET_QUESTION, m_userId, json);
}

void NetworkClient::sendSubmitAnswer(qint64 sessionId, int round, const QString &answer)
{
    QJsonObject obj;
    obj["session_id"] = sessionId;
    obj["round"] = round;
    obj["answer"] = answer;

    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);

    sendPacket(CMD_REQ_SUBMIT_ANSWER, m_userId, json);
}

void NetworkClient::sendUseLifeline(qint64 sessionId, int round)
{
    QJsonObject obj;
    obj["session_id"] = sessionId;
    obj["round"] = round;

    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);

    sendPacket(CMD_REQ_USE_LIFELINE, m_userId, json);
}

// 1vN Mode methods
void NetworkClient::sendCreateRoom(int easyCount, int mediumCount, int hardCount)
{
    QJsonObject obj;
    obj["easy_count"] = easyCount;
    obj["medium_count"] = mediumCount;
    obj["hard_count"] = hardCount;

    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);

    sendPacket(CMD_REQ_CREATE_ROOM, m_userId, json);
}

void NetworkClient::sendJoinRoom(qint64 roomId)
{
    QJsonObject obj;
    obj["room_id"] = roomId;

    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);

    sendPacket(CMD_REQ_JOIN_ROOM, m_userId, json);
}

void NetworkClient::sendLeaveRoom(qint64 roomId)
{
    QJsonObject obj;
    obj["room_id"] = roomId;

    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);

    sendPacket(CMD_REQ_LEAVE_ROOM, m_userId, json);
}

void NetworkClient::sendStartGame1VN(qint64 roomId)
{
    QJsonObject obj;
    obj["room_id"] = roomId;

    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);

    sendPacket(CMD_REQ_START_GAME, m_userId, json);
}

void NetworkClient::sendSubmitAnswer1VN(qint64 sessionId, int round, const QString &answer, double timeLeft)
{
    QJsonObject obj;
    obj["session_id"] = sessionId;
    obj["round"] = round;
    obj["answer"] = answer;
    obj["time_left"] = timeLeft;

    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);

    sendPacket(CMD_REQ_SUBMIT_ANSWER_1VN, m_userId, json);
}

void NetworkClient::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    qDebug() << "Received data:" << data.size() << "bytes";
    m_buffer.append(data);
    processBuffer();
}

void NetworkClient::processBuffer()
{
    while (m_buffer.size() >= 8) {  // Minimum header size
        // Read header
        QDataStream stream(m_buffer);
        stream.setByteOrder(QDataStream::BigEndian);

        quint16 cmd, user_id;
        quint32 length;
        stream >> cmd >> user_id >> length;

        qDebug() << "Processing packet: cmd=" << QString::number(cmd, 16) 
                 << "user_id=" << user_id << "length=" << length 
                 << "buffer_size=" << m_buffer.size();

        // Check if we have complete packet
        if (m_buffer.size() < 8 + static_cast<int>(length)) {
            qDebug() << "Waiting for more data. Need:" << (8 + length) << "Have:" << m_buffer.size();
            // Wait for more data
            return;
        }

        // Extract payload
        QByteArray payload = m_buffer.mid(8, static_cast<int>(length));
        m_buffer.remove(0, 8 + static_cast<int>(length));

        qDebug() << "Payload:" << payload;

        // Parse and emit signal
        parsePacket(cmd, payload);
    }
}

void NetworkClient::parsePacket(quint16 cmd, const QByteArray &jsonData)
{
    qDebug() << "Parsing packet: cmd=0x" << QString::number(cmd, 16) << "jsonData=" << jsonData;
    
    QJsonObject obj;
    QJsonDocument doc;
    
    // Handle empty JSON (server may return empty response for success)
    if (jsonData.isEmpty() || jsonData.trimmed().isEmpty()) {
        qDebug() << "Received empty JSON data for cmd=0x" << QString::number(cmd, 16);
        // For register, empty response typically means success
        if (cmd == CMD_RES_REGISTER) {
            qDebug() << "Empty register response - treating as success";
            emit registerResponse(true, "");
            return;
        }
        // For login, empty response might mean success but we need token
        if (cmd == CMD_RES_LOGIN) {
            qDebug() << "WARNING: Empty login response - this should not happen";
            qDebug() << "Treating as login failure";
            emit loginResponse(false, "", "Phản hồi trống từ server");
            return;
        }
        // For friend request commands, empty response means success
        if (cmd == CMD_RES_ADD_FRIEND) {
            qDebug() << "Empty ADD_FRIEND response - treating as success";
            emit addFriendResult(true, "");
            return;
        }
        if (cmd == CMD_RES_RESPOND_FRIEND) {
            qDebug() << "Empty RESPOND_FRIEND response - treating as success";
            emit respondFriendResult(true, "");
            return;
        }
        if (cmd == CMD_RES_REMOVE_FRIEND) {
            qDebug() << "Empty REMOVE_FRIEND response - treating as success";
            emit removeFriendResult(true, "");
            return;
        }
        // For other commands, create empty object
        obj = QJsonObject();
    } else {
        QJsonParseError error;
        doc = QJsonDocument::fromJson(jsonData, &error);

        if (error.error != QJsonParseError::NoError) {
            qDebug() << "JSON parse error:" << error.errorString();
            qDebug() << "JSON data:" << jsonData;
            // For register, if parse fails but we got a response, might be success
            if (cmd == CMD_RES_REGISTER) {
                qDebug() << "Register response parse failed - treating as success";
                emit registerResponse(true, "");
            } else {
                emit errorOccurred("Lỗi định dạng phản hồi từ server: " + error.errorString());
            }
            return;
        }

        // Handle both object and array responses
        if (doc.isObject()) {
            obj = doc.object();
        } else if (doc.isArray()) {
            // For array responses, create empty object (will handle in switch)
            obj = QJsonObject();
        }
    }

    if (doc.isArray()) {
        qDebug() << "Received packet: cmd=0x" << QString::number(cmd, 16) << "isArray: true data=" << doc.array();
    } else {
        qDebug() << "Received packet: cmd=0x" << QString::number(cmd, 16) << "isArray: false data=" << obj;
    }

    switch (cmd) {
        case CMD_RES_REGISTER:
            if (obj.contains("error")) {
                emit registerResponse(false, obj["error"].toString());
            } else {
                emit registerResponse(true, "");
            }
            break;

        case CMD_RES_LOGIN:
            qDebug() << "=== CMD_RES_LOGIN received ===";
            qDebug() << "JSON data (raw):" << jsonData;
            qDebug() << "Is array:" << doc.isArray();
            qDebug() << "Is object:" << doc.isObject();
            qDebug() << "Object keys:" << obj.keys();
            qDebug() << "Received LOGIN response:" << obj;
            
            if (obj.contains("error")) {
                QString errorMsg = obj["error"].toString();
                qDebug() << "Login failed with error:" << errorMsg;
                emit loginResponse(false, "", errorMsg);
            } else if (obj.contains("token")) {
                m_token = obj["token"].toString();
                m_loggedIn = true;
                // Extract user_id if available
                if (obj.contains("user_id")) {
                    m_userId = static_cast<quint16>(obj["user_id"].toInt());
                    qDebug() << "Login successful: user_id=" << m_userId << "token=" << m_token;
                } else {
                    qDebug() << "Login successful but no user_id in response";
                }
                qDebug() << "Emitting loginResponse(true, token, \"\")";
                emit loginResponse(true, m_token, "");
            } else {
                qDebug() << "ERROR: Login response missing both token and error fields";
                qDebug() << "Response object:" << obj;
                qDebug() << "Response JSON:" << jsonData;
                emit loginResponse(false, "", "Phản hồi không hợp lệ từ server");
            }
            break;

        case CMD_RES_LOGOUT:
            m_loggedIn = false;
            m_token.clear();
            m_userId = 0;
            // Disconnect socket after logout to allow clean reconnection
            if (m_socket->state() == QAbstractSocket::ConnectedState) {
                qDebug() << "Logout successful, disconnecting socket for clean reconnection";
                m_socket->disconnectFromHost();
            }
            emit logoutResponse(true);
            break;

        // QuickMode responses
        case CMD_NOTIFY_GAME_START:
            if (obj.contains("session_id") && obj.contains("total_rounds")) {
                qint64 sessionId = obj["session_id"].toVariant().toLongLong();
                int totalRounds = obj["total_rounds"].toInt();
                qDebug() << "=== CMD_NOTIFY_GAME_START ===";
                qDebug() << "session_id (qint64):" << sessionId;
                qDebug() << "total_rounds:" << totalRounds;
                // Reset duplicate tracking when new game starts
                m_lastQuestionSessionId = 0;
                m_lastQuestionRound = 0;
                m_lastQuestionId = 0;
                emit quickModeGameStart(sessionId, totalRounds);
            }
            break;

        case CMD_NOTIFY_QUESTION:
            qDebug() << "CMD_NOTIFY_QUESTION received, checking fields...";
            qDebug() << "Has session_id:" << obj.contains("session_id");
            qDebug() << "Has round:" << obj.contains("round");
            qDebug() << "Has content:" << obj.contains("content");
            qDebug() << "Has options:" << obj.contains("options");
            if (obj.contains("session_id") && obj.contains("round") && 
                obj.contains("content") && obj.contains("options")) {
                qint64 sessionId = obj["session_id"].toVariant().toLongLong();
                int round = obj["round"].toInt();
                qint64 questionId = obj["question_id"].toVariant().toLongLong();
                
                // SOLUTION 1: Prevent duplicate at NetworkClient level (early blocking)
                if (sessionId == m_lastQuestionSessionId && 
                    round == m_lastQuestionRound && 
                    questionId == m_lastQuestionId) {
                    qDebug() << "[NetworkClient] Duplicate question packet detected and blocked. "
                             << "sessionId=" << sessionId << " round=" << round 
                             << " questionId=" << questionId;
                    break;  // Don't emit signal
                }
                
                // Update tracking
                m_lastQuestionSessionId = sessionId;
                m_lastQuestionRound = round;
                m_lastQuestionId = questionId;
                
                QString content = obj["content"].toString();
                QJsonObject options = obj["options"].toObject();
                QString difficulty = obj["difficulty"].toString();
                qDebug() << "Emitting quickModeQuestionReceived: sessionId=" << sessionId 
                         << "round=" << round << "content=" << content;
                emit quickModeQuestionReceived(sessionId, round, questionId, content, options, difficulty);
            } else {
                qDebug() << "CMD_NOTIFY_QUESTION missing required fields!";
            }
            break;

        case CMD_RES_SUBMIT_ANSWER:
            // Check for error first
            if (obj.contains("error")) {
                QString errorMsg = obj["error"].toString();
                qDebug() << "Submit answer error:" << errorMsg;
                emit errorOccurred(errorMsg);
                // Still try to emit answer result if possible, but with error flag
                // This allows UI to handle the error properly
            } else if (obj.contains("session_id") && obj.contains("round")) {
                qint64 sessionId = obj["session_id"].toVariant().toLongLong();
                int round = obj["round"].toInt();
                bool correct = obj["correct"].toBool();
                QString correctAnswer = obj["correct_answer"].toString();
                int score = obj["score"].toInt();
                bool gameOver = obj["game_over"].toBool();
                emit quickModeAnswerResult(sessionId, round, correct, correctAnswer, score, gameOver);
            } else {
                qDebug() << "CMD_RES_SUBMIT_ANSWER missing required fields:" << obj;
            }
            break;

        case CMD_NOTIFY_ANSWER_RESULT:
            // Additional notification, can be ignored or used for UI updates
            break;

        case CMD_NOTIFY_GAME_OVER:
            if (obj.contains("session_id")) {
                qint64 sessionId = obj["session_id"].toVariant().toLongLong();
                int finalScore = obj["final_score"].toInt();
                int totalRounds = obj["total_rounds"].toInt();
                QString status = obj["status"].toString();
                bool win = obj["win"].toBool();
                emit quickModeGameOver(sessionId, finalScore, totalRounds, status, win);
            }
            break;

        case CMD_RES_USE_LIFELINE:
            if (obj.contains("session_id") && obj.contains("round") && 
                obj.contains("remaining_options") && obj.contains("removed_options")) {
                qint64 sessionId = obj["session_id"].toVariant().toLongLong();
                int round = obj["round"].toInt();
                QStringList remainingOptions;
                QJsonArray remainingArr = obj["remaining_options"].toArray();
                for (const QJsonValue &val : remainingArr) {
                    remainingOptions << val.toString();
                }
                QStringList removedOptions;
                QJsonArray removedArr = obj["removed_options"].toArray();
                for (const QJsonValue &val : removedArr) {
                    removedOptions << val.toString();
                }
                int remaining = obj["lifeline_remaining"].toInt();
                emit quickModeLifelineResult(sessionId, round, remainingOptions, removedOptions, remaining);
            }
            break;

        // Room responses
        case CMD_RES_CREATE_ROOM:
            if (obj.contains("error")) {
                emit errorOccurred("Tạo phòng thất bại: " + obj["error"].toString());
            } else if (obj.contains("room_id")) {
                qint64 roomId = obj["room_id"].toVariant().toLongLong();
                // If response contains members, emit room update
                if (obj.contains("members")) {
                    QJsonArray members = obj["members"].toArray();
                    emit oneVNRoomUpdate(members);
                }
                emit oneVNRoomCreated(roomId);
            }
            break;

        case CMD_RES_JOIN_ROOM:
            if (obj.contains("error")) {
                emit oneVNRoomJoined(false, obj["error"].toString());
            } else {
                // If response contains members, emit room update
                if (obj.contains("members")) {
                    QJsonArray members = obj["members"].toArray();
                    emit oneVNRoomUpdate(members);
                }
                emit oneVNRoomJoined(true, "");
            }
            break;

        case CMD_RES_LEAVE_ROOM:
            // Handle leave room response if needed
            break;

        case CMD_RES_START_GAME:
            if (obj.contains("error")) {
                emit errorOccurred("Bắt đầu game thất bại: " + obj["error"].toString());
            } else if (obj.contains("session_id") && obj.contains("room_id") && obj.contains("total_rounds")) {
                qint64 sessionId = obj["session_id"].toVariant().toLongLong();
                qint64 roomId = obj["room_id"].toVariant().toLongLong();
                int totalRounds = obj["total_rounds"].toInt();
                emit oneVNGameStart1VN(sessionId, roomId, totalRounds);
            }
            break;

        // 1vN game notifications
        case CMD_NOTIFY_GAME_START_1VN:
            if (obj.contains("session_id") && obj.contains("room_id") && obj.contains("total_rounds")) {
                qint64 sessionId = obj["session_id"].toVariant().toLongLong();
                qint64 roomId = obj["room_id"].toVariant().toLongLong();
                int totalRounds = obj["total_rounds"].toInt();
                emit oneVNGameStart1VN(sessionId, roomId, totalRounds);
            }
            break;

        case CMD_NOTIFY_QUESTION_1VN:
            qDebug() << "=== CMD_NOTIFY_QUESTION_1VN received ===";
            qDebug() << "Has round:" << obj.contains("round");
            qDebug() << "Has total_rounds:" << obj.contains("total_rounds");
            qDebug() << "Has difficulty:" << obj.contains("difficulty");
            qDebug() << "Has question_id:" << obj.contains("question_id");
            qDebug() << "Has content:" << obj.contains("content");
            qDebug() << "Has options:" << obj.contains("options");
            qDebug() << "Has time_limit:" << obj.contains("time_limit");
            if (obj.contains("round") && obj.contains("total_rounds") && 
                obj.contains("difficulty") && obj.contains("question_id") &&
                obj.contains("content") && obj.contains("options") && obj.contains("time_limit")) {
                int round = obj["round"].toInt();
                int totalRounds = obj["total_rounds"].toInt();
                QString difficulty = obj["difficulty"].toString();
                qint64 questionId = obj["question_id"].toVariant().toLongLong();
                QString content = obj["content"].toString();
                QJsonObject options = obj["options"].toObject();
                int timeLimit = obj["time_limit"].toInt();
                qDebug() << "Emitting oneVNQuestion1VNReceived - Round:" << round << "Content:" << content;
                qDebug() << "Options:" << options;
                emit oneVNQuestion1VNReceived(round, totalRounds, difficulty, questionId, content, options, timeLimit);
                qDebug() << "Signal emitted";
            } else {
                qDebug() << "ERROR: Missing required fields in question packet";
            }
            break;

        case CMD_RES_SUBMIT_ANSWER_1VN:
            if (obj.contains("correct") && obj.contains("score") && obj.contains("total_score") && obj.contains("eliminated")) {
                bool correct = obj["correct"].toBool();
                int score = obj["score"].toInt();
                int totalScore = obj["total_score"].toInt();
                bool eliminated = obj["eliminated"].toBool();
                bool timeout = obj.contains("timeout") && obj["timeout"].toBool();
                emit oneVNAnswerResult1VN(correct, score, totalScore, eliminated, timeout);
            }
            break;

        case CMD_NOTIFY_ELIMINATION:
            if (obj.contains("user_id") && obj.contains("round")) {
                qint64 userId = obj["user_id"].toVariant().toLongLong();
                int round = obj["round"].toInt();
                emit oneVNElimination(userId, round);
            }
            break;

        case CMD_NOTIFY_GAME_OVER_1VN:
            if (obj.contains("winner_id") && obj.contains("leaderboard")) {
                qint64 winnerId = obj["winner_id"].toVariant().toLongLong();
                QJsonArray leaderboard = obj["leaderboard"].toArray();
                emit oneVNGameOver1VN(winnerId, leaderboard);
            }
            break;

        case CMD_NOTIFY_ROOM_UPDATE:
            // Handle both room member updates and leaderboard updates
            if (obj.contains("members")) {
                QJsonArray members = obj["members"].toArray();
                emit oneVNRoomUpdate(members);
            } else if (obj.contains("leaderboard")) {
                // Leaderboard update during game - emit as room update
                // OneVNWindow will handle it based on current state
                QJsonArray leaderboard = obj["leaderboard"].toArray();
                emit oneVNRoomUpdate(leaderboard);
            }
            break;

        // Friends responses
        case CMD_RES_SEARCH_USER:
            // Server may return array directly or wrapped in object
            if (obj.contains("users")) {
                QJsonArray users = obj["users"].toArray();
                emit searchUserResult(users);
            } else if (doc.isArray()) {
                // Direct array response
                QJsonArray users = doc.array();
                emit searchUserResult(users);
            } else {
                emit searchUserResult(QJsonArray());
            }
            break;

        case CMD_RES_ADD_FRIEND:
            qDebug() << "=== CMD_RES_ADD_FRIEND received ===";
            qDebug() << "jsonData size:" << jsonData.size();
            qDebug() << "obj keys:" << obj.keys();
            if (obj.contains("error")) {
                qDebug() << "Error:" << obj["error"].toString();
                emit addFriendResult(false, obj["error"].toString());
            } else {
                qDebug() << "Success - emitting addFriendResult(true)";
                emit addFriendResult(true, "");
            }
            break;

        case CMD_RES_REMOVE_FRIEND:
            if (obj.contains("error")) {
                emit removeFriendResult(false, obj["error"].toString());
            } else {
                emit removeFriendResult(true, "");
            }
            break;

        case CMD_RES_GET_PENDING_REQ:
            if (doc.isArray()) {
                QJsonArray requests = doc.array();
                emit pendingRequestsReceived(requests);
            } else {
                emit pendingRequestsReceived(QJsonArray());
            }
            break;

        case CMD_RES_RESPOND_FRIEND:
            qDebug() << "=== CMD_RES_RESPOND_FRIEND received ===";
            qDebug() << "jsonData size:" << jsonData.size();
            qDebug() << "obj keys:" << obj.keys();
            if (obj.contains("error")) {
                qDebug() << "Error:" << obj["error"].toString();
                emit respondFriendResult(false, obj["error"].toString());
            } else {
                qDebug() << "Success - emitting respondFriendResult(true)";
                emit respondFriendResult(true, "");
            }
            break;

        case CMD_NOTIFY_FRIEND_REQ:
            if (obj.contains("from_user_id") && obj.contains("from_username")) {
                qint64 fromUserId = obj["from_user_id"].toVariant().toLongLong();
                QString fromUsername = obj["from_username"].toString();
                emit friendRequestReceived(fromUserId, fromUsername);
            }
            break;

        case CMD_RES_LIST_FRIENDS:
            // Server returns array directly from dao_friends_list
            if (doc.isArray()) {
                QJsonArray friends = doc.array();
                emit listFriendsResult(friends);
            } else if (obj.contains("friends")) {
                QJsonArray friends = obj["friends"].toArray();
                emit listFriendsResult(friends);
            } else {
                emit listFriendsResult(QJsonArray());
            }
            break;

        case CMD_RES_GET_FRIEND_INFO:
            if (obj.contains("error")) {
                emit errorOccurred("Lỗi lấy thông tin bạn bè: " + obj["error"].toString());
            } else {
                emit friendInfoReceived(obj);
            }
            break;

        case CMD_NOTIFY_FRIEND_STATUS:
            qDebug() << "=== CMD_NOTIFY_FRIEND_STATUS received ===";
            qDebug() << "Object:" << obj;
            if (obj.contains("user_id") && obj.contains("status")) {
                qint64 userId = obj["user_id"].toVariant().toLongLong();
                QString status = obj["status"].toString();
                qint64 roomId = obj.contains("room_id") ? obj["room_id"].toVariant().toLongLong() : 0;
                
                qDebug() << "Friend status changed - userId:" << userId;
                qDebug() << "status:" << status;
                qDebug() << "roomId:" << roomId;
                
                emit friendStatusChanged(userId, status, roomId);
            } else {
                qDebug() << "ERROR: CMD_NOTIFY_FRIEND_STATUS missing required fields";
            }
            break;

        // Chat responses
        case CMD_RES_SEND_DM:
            if (obj.contains("error")) {
                emit errorOccurred("Gửi tin nhắn thất bại: " + obj["error"].toString());
            }
            // Success - no action needed, notification will come via CMD_NOTIFY_DM
            break;

        case CMD_NOTIFY_DM:
            qDebug() << "=== CMD_NOTIFY_DM received ===";
            qDebug() << "Object:" << obj;
            if (obj.contains("from_user_id") && obj.contains("from_username") && obj.contains("message")) {
                qint64 fromUserId = obj["from_user_id"].toVariant().toLongLong();
                QString fromUsername = obj["from_username"].toString();
                QString message = obj["message"].toString();
                qint64 timestamp = obj.contains("timestamp") ? obj["timestamp"].toVariant().toLongLong() : 0;
                
                qDebug() << "Parsed DM - fromUserId:" << fromUserId;
                qDebug() << "fromUsername:" << fromUsername;
                qDebug() << "message:" << message;
                qDebug() << "timestamp:" << timestamp;
                
                emit dmReceived(fromUserId, fromUsername, message, timestamp);
            } else {
                qDebug() << "ERROR: CMD_NOTIFY_DM missing required fields";
                qDebug() << "Has from_user_id:" << obj.contains("from_user_id");
                qDebug() << "Has from_username:" << obj.contains("from_username");
                qDebug() << "Has message:" << obj.contains("message");
            }
            break;

        case CMD_RES_FETCH_OFFLINE:
            // Server may return array directly or wrapped in object
            if (doc.isArray()) {
                QJsonArray messages = doc.array();
                emit offlineMessagesReceived(messages);
            } else if (obj.contains("messages")) {
                QJsonArray messages = obj["messages"].toArray();
                emit offlineMessagesReceived(messages);
            } else {
                emit offlineMessagesReceived(QJsonArray());
            }
            break;

        // Stats responses
        case CMD_RES_GET_PROFILE:
            if (obj.contains("error")) {
                emit errorOccurred("Lỗi lấy thông tin profile: " + obj["error"].toString());
            } else {
                emit profileReceived(obj);
            }
            break;

        case CMD_RES_UPDATE_AVATAR:
            if (obj.contains("error")) {
                emit avatarUpdated(false, obj["error"].toString());
            } else if (obj.contains("success") && obj["success"].toBool()) {
                emit avatarUpdated(true, "Avatar đã được cập nhật");
                // Refresh profile to get updated avatar
                sendGetProfile();
            } else {
                emit avatarUpdated(false, "Lỗi không xác định");
            }
            break;

        case CMD_RES_LEADERBOARD:
            // Server returns array of players
            if (doc.isArray()) {
                QJsonArray leaderboard = doc.array();
                emit leaderboardReceived(leaderboard);
            } else if (obj.contains("leaderboard")) {
                QJsonArray leaderboard = obj["leaderboard"].toArray();
                emit leaderboardReceived(leaderboard);
            } else {
                emit leaderboardReceived(QJsonArray());
            }
            break;

        default:
            qDebug() << "Unknown command:" << QString::number(cmd, 16);
            break;
    }
}

void NetworkClient::onSocketError(QAbstractSocket::SocketError error)
{
    QString errorMsg;
    switch (error) {
        case QAbstractSocket::ConnectionRefusedError:
            errorMsg = "Kết nối bị từ chối. Kiểm tra server có đang chạy không.";
            break;
        case QAbstractSocket::HostNotFoundError:
            errorMsg = "Không tìm thấy server. Kiểm tra địa chỉ host.";
            break;
        case QAbstractSocket::NetworkError:
            errorMsg = "Lỗi mạng. Kiểm tra kết nối internet.";
            break;
        case QAbstractSocket::RemoteHostClosedError:
            // Server closed connection - this is normal after response
            // Try to read any remaining data first
            if (m_socket->bytesAvailable() > 0) {
                qDebug() << "Server closed but data available, reading...";
                onReadyRead();
            }
            // Don't emit error if we're logged in (server closed after successful login)
            if (!m_loggedIn) {
                errorMsg = "Server đã đóng kết nối.";
                emit errorOccurred(errorMsg);
            }
            return;  // Don't emit error if logged in
        default:
            errorMsg = "Lỗi kết nối: " + m_socket->errorString();
            break;
    }
    emit errorOccurred(errorMsg);
}

void NetworkClient::onSocketStateChanged(QAbstractSocket::SocketState state)
{
    qDebug() << "Socket state changed:" << state;
    if (state == QAbstractSocket::ConnectedState) {
        emit connected();
    } else if (state == QAbstractSocket::UnconnectedState) {
        qDebug() << "Socket disconnected";
        
        // Try to read any remaining data before disconnecting
        if (m_socket->bytesAvailable() > 0) {
            qDebug() << "Reading remaining data before disconnect:" << m_socket->bytesAvailable() << "bytes";
            onReadyRead();
        }
        
        emit disconnected();
        
        // Only clear login state if we didn't successfully login
        // (server closes connection after response, which is normal)
        if (!m_loggedIn) {
            m_token.clear();
        }
    }
}

// Friends & Chat methods
void NetworkClient::sendSearchUser(const QString &query, int limit) {
    QJsonObject obj;
    obj["query"] = query;
    obj["limit"] = limit;

    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);

    sendPacket(CMD_REQ_SEARCH_USER, m_userId, json);
}

void NetworkClient::sendAddFriend(qint64 friendId) {
    QJsonObject obj;
    obj["friend_id"] = friendId;

    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);

    sendPacket(CMD_REQ_ADD_FRIEND, m_userId, json);
}

void NetworkClient::sendRemoveFriend(qint64 friendId) {
    QJsonObject obj;
    obj["friend_id"] = friendId;

    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);

    sendPacket(CMD_REQ_REMOVE_FRIEND, m_userId, json);
}

void NetworkClient::sendListFriends() {
    QJsonObject obj;
    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);

    sendPacket(CMD_REQ_LIST_FRIENDS, m_userId, json);
}

void NetworkClient::sendGetFriendInfo(qint64 friendId) {
    QJsonObject obj;
    obj["friend_id"] = friendId;

    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);

    sendPacket(CMD_REQ_GET_FRIEND_INFO, m_userId, json);
}

void NetworkClient::sendSendDM(qint64 toUserId, const QString &message) {
    qDebug() << "=== sendSendDM ===";
    qDebug() << "toUserId:" << toUserId;
    qDebug() << "message:" << message;
    qDebug() << "myUserId:" << m_userId;
    
    QJsonObject obj;
    obj["to_user_id"] = toUserId;
    obj["message"] = message;

    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);

    qDebug() << "Sending packet CMD_REQ_SEND_DM with JSON:" << json;
    sendPacket(CMD_REQ_SEND_DM, m_userId, json);
}

void NetworkClient::sendFetchOfflineMessages(qint64 friendId) {
    QJsonObject obj;
    if (friendId > 0) {
        obj["friend_id"] = friendId;
    }
    
    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);
    
    sendPacket(CMD_REQ_FETCH_OFFLINE, m_userId, json);
}

// Stats methods
void NetworkClient::sendGetProfile()
{
    QJsonObject obj;  // Empty object - server uses session user_id
    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);
    sendPacket(CMD_REQ_GET_PROFILE, m_userId, json);
}

void NetworkClient::sendLeaderboard()
{
    QJsonObject obj;  // Empty object - server returns top 20 by default
    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);
    sendPacket(CMD_REQ_LEADERBOARD, m_userId, json);
}

void NetworkClient::sendUpdateAvatar(const QString &avatarPath)
{
    QJsonObject obj;
    obj["avatar_path"] = avatarPath;
    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);
    sendPacket(CMD_REQ_UPDATE_AVATAR, m_userId, json);
}

void NetworkClient::sendGetPendingRequests() {
    QJsonObject obj;
    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);

    sendPacket(CMD_REQ_GET_PENDING_REQ, m_userId, json);
}

void NetworkClient::sendRespondFriend(qint64 fromUserId, bool accept) {
    QJsonObject obj;
    obj["from_user_id"] = fromUserId;
    obj["accept"] = accept ? "true" : "false";

    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);

    sendPacket(CMD_REQ_RESPOND_FRIEND, m_userId, json);
}

