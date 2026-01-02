#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>

class NetworkClient : public QObject
{
    Q_OBJECT

public:
    explicit NetworkClient(QObject *parent = nullptr);
    ~NetworkClient();

    Q_INVOKABLE bool connectToServer(const QString &host, quint16 port);
    Q_INVOKABLE void disconnectFromServer();
    Q_INVOKABLE bool isConnected() const;
    Q_INVOKABLE bool isLoggedIn() const;
    Q_INVOKABLE QString getToken() const;

    // Send requests
    Q_INVOKABLE void sendRegister(const QString &username, const QString &password);
    Q_INVOKABLE void sendLogin(const QString &username, const QString &password);
    Q_INVOKABLE void sendLogout();
    
    // QuickMode requests
    Q_INVOKABLE void sendStartQuickMode();
    Q_INVOKABLE void sendGetQuestion(qint64 sessionId, int round);
    Q_INVOKABLE void sendSubmitAnswer(qint64 sessionId, int round, const QString &answer);
    Q_INVOKABLE void sendUseLifeline(qint64 sessionId, int round);
    
    // 1vN Mode requests
    Q_INVOKABLE void sendCreateRoom(int easyCount, int mediumCount, int hardCount);
    Q_INVOKABLE void sendJoinRoom(qint64 roomId);
    Q_INVOKABLE void sendLeaveRoom(qint64 roomId);
    Q_INVOKABLE void sendStartGame1VN(qint64 roomId);
    Q_INVOKABLE void sendSubmitAnswer1VN(qint64 sessionId, int round, const QString &answer, double timeLeft);
    
    // Friends & Chat requests
    Q_INVOKABLE void sendSearchUser(const QString &query, int limit = 20);
    Q_INVOKABLE void sendAddFriend(qint64 friendId);
    Q_INVOKABLE void sendRemoveFriend(qint64 friendId);
    Q_INVOKABLE void sendListFriends();
    Q_INVOKABLE void sendGetFriendInfo(qint64 friendId);
    Q_INVOKABLE void sendGetPendingRequests();
    Q_INVOKABLE void sendRespondFriend(qint64 fromUserId, bool accept);
    Q_INVOKABLE void sendSendDM(qint64 toUserId, const QString &message);
    Q_INVOKABLE void sendRoomChat(qint64 roomId, const QString &message);
    Q_INVOKABLE void sendFetchOfflineMessages(qint64 friendId = 0);
    
    // Invite friends to room
    Q_INVOKABLE void sendInviteFriend(qint64 roomId, qint64 friendId);
    Q_INVOKABLE void sendRespondInvite(qint64 roomId, bool accept);
    
    // Stats requests
    Q_INVOKABLE void sendGetProfile();
    Q_INVOKABLE void sendLeaderboard();
    Q_INVOKABLE void sendUpdateAvatar(const QString &avatarPath);
    Q_INVOKABLE void sendGetOneVNHistory();
    Q_INVOKABLE void sendGetReplayDetails(qint64 sessionId);
    Q_INVOKABLE void sendListRooms();
    
    Q_INVOKABLE quint16 getUserId() const;

signals:
    void connected();
    void disconnected();
    void registerResponse(bool success, const QString &error);
    void loginResponse(bool success, const QString &token, const QString &error);
    void logoutResponse(bool success);
    void errorOccurred(const QString &error);
    
    // QuickMode signals
    void quickModeGameStart(qint64 sessionId, int totalRounds);
    void quickModeQuestionReceived(qint64 sessionId, int round, qint64 questionId, 
                                   const QString &content, const QJsonObject &options, 
                                   const QString &difficulty);
    void quickModeAnswerResult(qint64 sessionId, int round, bool correct, 
                               const QString &correctAnswer, int score, bool gameOver);
    void quickModeGameOver(qint64 sessionId, int finalScore, int totalRounds, 
                          const QString &status, bool win);
    void quickModeLifelineResult(qint64 sessionId, int round, 
                                const QStringList &remainingOptions, 
                                const QStringList &removedOptions, int remaining);
    
    // 1vN Mode signals
    void oneVNRoomCreated(qint64 roomId);
    void oneVNRoomJoined(bool success, qint64 roomId, const QString &error);
    void oneVNRoomUpdate(const QJsonArray &members);
    void oneVNRoomClosed(qint64 roomId, const QString &reason);
    void oneVNGameStart1VN(qint64 sessionId, qint64 roomId, int totalRounds);
    void oneVNQuestion1VNReceived(int round, int totalRounds, const QString &difficulty,
                                   qint64 questionId, const QString &content,
                                   const QJsonObject &options, int timeLimit);
    void oneVNAnswerResult1VN(bool correct, int score, int totalScore, bool eliminated, bool timeout = false);
    void oneVNElimination(qint64 userId, int round);
    void oneVNGameOver1VN(qint64 winnerId, const QJsonArray &leaderboard);
    
    // Friends & Chat signals
    void searchUserResult(const QJsonArray &users);
    void addFriendResult(bool success, const QString &error);
    void removeFriendResult(bool success, const QString &error);
    void listFriendsResult(const QJsonArray &friends);
    void friendInfoReceived(const QJsonObject &friendInfo);
    void pendingRequestsReceived(const QJsonArray &requests);
    void respondFriendResult(bool success, const QString &error);
    void friendRequestReceived(qint64 fromUserId, const QString &fromUsername);
    void dmReceived(qint64 fromUserId, const QString &fromUsername, const QString &message, qint64 timestamp);
    void roomChatReceived(qint64 userId, const QString &username, const QString &message, qint64 timestamp);
    void offlineMessagesReceived(const QJsonArray &messages);
    void friendStatusChanged(qint64 userId, const QString &status, qint64 roomId);
    
    // Invite signals
    void inviteFriendResult(bool success, const QString &error);
    void roomInviteReceived(qint64 roomId, qint64 fromUserId, const QString &fromUsername);
    void respondInviteResult(bool success, qint64 roomId, const QString &error);
    
    // Stats signals
    void profileReceived(const QJsonObject &profile);
    void leaderboardReceived(const QJsonArray &leaderboard);
    void avatarUpdated(bool success, const QString &message);
    void oneVNHistoryReceived(const QJsonArray &history);
    void replayDetailsReceived(const QJsonObject &replayData);
    void roomsListReceived(const QJsonArray &rooms);

private slots:
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);
    void onSocketStateChanged(QAbstractSocket::SocketState state);

private:
    QTcpSocket *m_socket;
    QString m_token;
    bool m_loggedIn;
    quint16 m_userId;  // User ID from login
    QByteArray m_buffer;  // Buffer for incomplete packets
    
    // Duplicate prevention tracking for questions
    qint64 m_lastQuestionSessionId;
    int m_lastQuestionRound;
    qint64 m_lastQuestionId;

    // Packet handling
    void sendPacket(quint16 cmd, quint16 user_id, const QByteArray &json);
    void parsePacket(quint16 cmd, const QByteArray &jsonData);
    QByteArray createPacketHeader(quint16 cmd, quint16 user_id, quint32 length);
    void processBuffer();
};

#endif // NETWORKCLIENT_H

