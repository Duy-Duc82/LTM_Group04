import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: chatScreen
    width: parent ? parent.width : 600
    height: parent ? parent.height : 700
    
    property StackView stackView
    property string username: ""
    property int friendId: 0
    property string friendName: ""
    
    // Messages list
    property var messages: []
    
    // Connect to global NetworkClient instance (registered in main.cpp)
    Connections {
        target: networkClient
        
        function onDmReceived(fromUserId, fromUsername, message, timestamp) {
            console.log("=== ChatScreen: dmReceived ===")
            console.log("From:", fromUserId, fromUsername, "Message:", message)
            console.log("Timestamp:", timestamp)
            console.log("friendId:", friendId, "myUserId:", networkClient.getUserId())
            
            var myUserId = networkClient.getUserId()
            var isFromFriend = (fromUserId === friendId)
            var isFromMe = (fromUserId === myUserId)
            
            // Add message if:
            // 1. Message is from current friend (friendId)
            // 2. OR message is from me (when chatting with this friend)
            if (isFromFriend || (isFromMe && friendId > 0)) {
                console.log("Message is valid, checking for duplicates...")
                
                // Check if message already exists (avoid duplicates from optimistic update)
                // Compare: text, senderId, and timestamp (within 5 seconds)
                var exists = false
                var currentTime = timestamp || Math.floor(Date.now() / 1000)
                
                for (var i = 0; i < messages.length; i++) {
                    var msg = messages[i]
                    var msgTime = msg.timestampRaw || 0
                    
                    // If same text, same sender, and timestamp within 5 seconds → duplicate
                    if (msg.text === message && 
                        msg.senderId === fromUserId &&
                        Math.abs(msgTime - currentTime) < 5) {
                        console.log("Duplicate message detected, skipping")
                        exists = true
                        break
                    }
                }
                
                if (!exists) {
                    console.log("Adding new message to list")
                    var date = new Date(timestamp * 1000)
                    var timeStr = date.toLocaleTimeString("vi-VN", { hour: "2-digit", minute: "2-digit" })
                    
                    var newMessage = {
                        "id": messages.length + 1,
                        "senderId": fromUserId,
                        "senderName": fromUsername,
                        "text": message,
                        "timestamp": timeStr,
                        "timestampRaw": timestamp  // Store raw timestamp for duplicate detection
                    }
                    // Create new array to trigger ListView update
                    var newMessages = messages.slice()
                    newMessages.push(newMessage)
                    messages = newMessages  // Force update with new array reference
                    console.log("Message added, total messages:", messages.length)
                    
                    // Scroll to bottom after a short delay to ensure UI is updated
                    Qt.callLater(function() {
                        messagesListView.positionViewAtEnd()
                    })
                }
            } else {
                console.log("Message ignored - not from friend or not from me to this friend")
                console.log("isFromFriend:", isFromFriend, "isFromMe:", isFromMe, "friendId:", friendId)
            }
        }
        
        function onOfflineMessagesReceived(offlineMessages) {
            console.log("=== ChatScreen: offlineMessagesReceived ===")
            console.log("Messages count:", offlineMessages.length)
            console.log("Messages data:", JSON.stringify(offlineMessages))
            
            // Add offline messages from this friend
            var tempMessages = messages.slice()
            var myUserId = networkClient.getUserId()
            
            for (var i = 0; i < offlineMessages.length; i++) {
                var msg = offlineMessages[i]
                // Server returns: {"id": ..., "sender_id": ..., "message": ..., "created_at": "..."}
                var senderId = msg.sender_id || msg.senderId || 0
                
                // Check if message is from or to current friend
                if (senderId === friendId || senderId === myUserId) {
                    // Parse timestamp from created_at string
                    var createdAt = msg.created_at || msg.createdAt || ""
                    var timeStr = createdAt
                    if (createdAt) {
                        // Try to parse ISO format or PostgreSQL timestamp
                        var date = new Date(createdAt)
                        if (!isNaN(date.getTime())) {
                            timeStr = date.toLocaleTimeString("vi-VN", { hour: "2-digit", minute: "2-digit" })
                        } else {
                            // Fallback: use as-is or extract time part
                            var parts = createdAt.split(" ")
                            if (parts.length > 1) {
                                timeStr = parts[1].substring(0, 5)  // HH:MM
                            }
                        }
                    }
                    
                    tempMessages.push({
                        "id": msg.id || tempMessages.length + 1,
                        "senderId": senderId,
                        "senderName": senderId === friendId ? friendName : username,
                        "text": msg.message || msg.text || "",
                        "timestamp": timeStr
                    })
                }
            }
            messages = tempMessages
        }
    }
    
    Component.onCompleted: {
        // Fetch conversation (both sent and received messages) when screen is shown
        if (friendId > 0) {
            networkClient.sendFetchOfflineMessages(friendId)
        } else {
            networkClient.sendFetchOfflineMessages()
        }
    }
    
    // Background
    Rectangle {
        anchors.fill: parent
        color: "#2E1A47"
    }
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // Header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: Qt.rgba(0.0, 0.0, 0.0, 0.3)
            z: 10  // Ensure header is above ScrollView
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 15
                z: 11  // Ensure RowLayout is above ScrollView
                
                Button {
                    text: "←"
                    font.pixelSize: 20
                    flat: true
                    z: 12  // Ensure button is above other elements
                    enabled: true
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                    
                    background: Rectangle {
                        color: parent.hovered ? Qt.rgba(1.0, 1.0, 1.0, 0.2) : "transparent"
                        radius: 4
                    }
                    
                    onClicked: {
                        console.log("Back button clicked")
                        console.log("stackView:", stackView)
                        console.log("StackView depth:", stackView ? stackView.depth : "null")
                        if (stackView) {
                            var result = stackView.pop()
                            console.log("pop() result:", result)
                        } else {
                            console.log("ERROR: stackView is null!")
                        }
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: "#FFFFFF"
                    }
                }
                
                Text {
                    Layout.fillWidth: true
                    text: friendName
                    font.family: "Lexend"
                    font.pixelSize: 18
                    font.bold: true
                    color: "#FFFFFF"
                }
            }
        }
        
        // Messages area
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            
            ListView {
                id: messagesListView
                width: parent.width
                model: messages
                spacing: 10
                anchors.margins: 15
                
                // Auto-scroll to bottom when new message is added
                onCountChanged: {
                    if (count > 0) {
                        Qt.callLater(function() {
                            messagesListView.positionViewAtEnd()
                        })
                    }
                }
                
                delegate: Item {
                    width: messagesListView.width - 30
                    implicitHeight: messageBubble.height + 20
                    
                    property bool isMyMessage: {
                        var myUserId = networkClient.getUserId()
                        return modelData.senderId === myUserId || modelData.senderId === 0
                    }
                    
                    RowLayout {
                        id: messageRow
                        anchors.fill: parent
                        spacing: 10
                        
                        // Spacer for received messages (left side)
                        Item {
                            Layout.fillWidth: isMyMessage
                            Layout.preferredWidth: !isMyMessage ? 0 : undefined
                        }
                        
                        Rectangle {
                            id: messageBubble
                            Layout.preferredWidth: Math.min(messageText.implicitWidth + 40, messagesListView.width * 0.7)
                            Layout.preferredHeight: messageText.implicitHeight + timestampText.implicitHeight + 30
                            radius: 15
                            // Đổi màu: cả tin nhắn gửi và nhận đều màu vàng
                            color: "#FFC107"
                            
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 5
                                
                                Text {
                                    id: messageText
                                    Layout.fillWidth: true
                                    text: modelData.text
                                    font.family: "Lexend"
                                    font.pixelSize: 14
                                    // Đổi màu text: cả tin nhắn gửi và nhận đều màu đen
                                    color: "#000000"
                                    wrapMode: Text.WordWrap
                                }
                                
                                Text {
                                    id: timestampText
                                    Layout.alignment: isMyMessage ? Qt.AlignLeft : Qt.AlignRight
                                    text: modelData.timestamp
                                    font.family: "Lexend"
                                    font.pixelSize: 10
                                    // Đổi màu timestamp
                                    color: isMyMessage ? Qt.rgba(1.0, 1.0, 1.0, 0.7) : "#666666"
                                }
                            }
                        }
                        
                        // Spacer for sent messages (right side)
                        Item {
                            Layout.fillWidth: !isMyMessage
                            Layout.preferredWidth: isMyMessage ? 0 : undefined
                        }
                    }
                }
            }
        }
        
        // Input area
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 70
            color: Qt.rgba(0.0, 0.0, 0.0, 0.3)
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10
                
                TextField {
                    id: messageInput
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    placeholderText: "Nhập tin nhắn..."
                    font.family: "Lexend"
                    font.pixelSize: 14
                    color: "#FFFFFF"
                    
                    background: Rectangle {
                        color: Qt.rgba(1.0, 1.0, 1.0, 0.1)
                        radius: 25
                    }
                    
                    onAccepted: {
                        sendMessage()
                    }
                }
                
                Button {
                    Layout.preferredWidth: 50
                    Layout.preferredHeight: 50
                    text: "➤"
                    font.pixelSize: 20
                    
                    background: Rectangle {
                        color: parent.enabled ? "#FFC107" : "#666666"
                        radius: 25
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: "#000000"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    enabled: messageInput.text.trim().length > 0
                    
                    onClicked: {
                        sendMessage()
                    }
                }
            }
        }
    }
    
    function sendMessage() {
        var text = messageInput.text.trim()
        if (text.length === 0) return
        
        console.log("=== sendMessage ===")
        console.log("Text:", text)
        console.log("friendId:", friendId)
        console.log("myUserId:", networkClient.getUserId())
        
        var myUserId = networkClient.getUserId()
        var currentTime = Math.floor(Date.now() / 1000)  // Unix timestamp in seconds
        
        // Add message to list immediately (optimistic update)
        var newMessage = {
            "id": messages.length + 1,
            "senderId": myUserId,  // Use actual user ID instead of 0
            "senderName": username,
            "text": text,
            "timestamp": new Date().toLocaleTimeString("vi-VN", { hour: "2-digit", minute: "2-digit" }),
            "timestampRaw": currentTime  // Store raw timestamp for duplicate detection
        }
        // Create new array to trigger ListView update
        var newMessages = messages.slice()
        newMessages.push(newMessage)
        messages = newMessages  // Force update with new array reference
        
        messageInput.text = ""
        
        // Scroll to bottom immediately for optimistic update
        Qt.callLater(function() {
            messagesListView.positionViewAtEnd()
        })
        
        // Send to server
        console.log("Sending DM to friendId:", friendId, "message:", text)
        networkClient.sendSendDM(friendId, text)
    }
}

