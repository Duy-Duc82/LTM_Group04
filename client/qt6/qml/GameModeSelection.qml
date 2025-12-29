import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: gameModeSelection
    width: parent ? parent.width : 600
    height: parent ? parent.height : 700
    
    property StackView stackView
    property string username: ""
    
    // Friends list from server
    property var friendsList: []
    
    // Connect to global NetworkClient instance (registered in main.cpp)
    Connections {
        target: networkClient
        
        function onListFriendsResult(friends) {
            console.log("=== listFriendsResult ===")
            console.log("Friends count:", friends.length)
            console.log("Friends data:", JSON.stringify(friends))
            
            var tempList = []
            for (var i = 0; i < friends.length; i++) {
                var friend = friends[i]
                // Handle both QJsonObject and plain JS object
                var userId = friend.user_id || friend.userId || 0
                var username = friend.username || ""
                var status = friend.status || "ACCEPTED"
                // Online status will be updated via friendStatusChanged signal
                var onlineStatus = friend.online_status || friend.onlineStatus || "offline"
                var isOnline = (onlineStatus === "online" || onlineStatus === "in_game")
                
                console.log("Friend", i, ":", userId, username, "status:", status, "online:", isOnline)
                
                tempList.push({
                    id: userId,
                    userId: userId,
                    username: username,
                    online: isOnline,
                    onlineStatus: onlineStatus,
                    status: status,
                    hasUnreadMessages: false  // Badge indicator for unread messages
                })
            }
            friendsList = tempList
            console.log("Updated friendsList length:", friendsList.length)
        }
        
        function onFriendStatusChanged(userId, status, roomId) {
            console.log("=== friendStatusChanged ===")
            console.log("UserId:", userId, "Status:", status, "roomId:", roomId)
            
            // Update friend status in list
            var updated = false
            for (var i = 0; i < friendsList.length; i++) {
                if (friendsList[i].id === userId || friendsList[i].userId === userId) {
                    var isOnline = (status === "online" || status === "in_game")
                    console.log("Updating friend", userId, "online status to:", isOnline)
                    friendsList[i].online = isOnline
                    friendsList[i].onlineStatus = status
                    updated = true
                    break
                }
            }
            
            // Force update by creating new array reference
            if (updated) {
                var newList = friendsList.slice()
                friendsList = newList
                console.log("Friends list updated, friend", userId, "is now", status)
            } else {
                console.log("Friend", userId, "not found in friends list")
            }
        }
        
        function onDmReceived(fromUserId, fromUsername, message, timestamp) {
            console.log("=== GameModeSelection: dmReceived ===")
            console.log("From:", fromUserId, "Message:", message)
            
            // Set unread badge for friend (only if not currently in ChatScreen with this friend)
            // Note: We don't know which ChatScreen is open, so we'll always set the badge
            // The badge will be reset when opening ChatScreen
            
            var updated = false
            for (var i = 0; i < friendsList.length; i++) {
                if (friendsList[i].id === fromUserId || friendsList[i].userId === fromUserId) {
                    friendsList[i].hasUnreadMessages = true
                    updated = true
                    console.log("Set unread badge for friend:", fromUserId)
                    break
                }
            }
            
            // Force update by creating new array reference
            if (updated) {
                var newList = friendsList.slice()
                friendsList = newList
            }
        }
        
    }
    
    Component.onCompleted: {
        // Load friends list when screen is shown
        networkClient.sendListFriends()
    }
    
    // Background gradient
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#667eea" }
            GradientStop { position: 1.0; color: "#764ba2" }
        }
    }
    
    RowLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20
        
        // Left side: Game modes
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 30
        
        Item { Layout.fillHeight: true }
        
        // Welcome label
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            radius: 10
            color: "white"
            
            Text {
                anchors.centerIn: parent
                text: "🎮 Chào mừng, " + username + "! 🎮"
                font.family: "Lexend"
                font.pixelSize: 18
                font.bold: true
                color: "#667eea"
            }
        }
        
        // Title
        Text {
            Layout.fillWidth: true
            text: "💰 AI LÀ TRIỆU PHÚ 💰"
            font.family: "Lexend"
            font.pixelSize: 28
            font.bold: true
            color: "#FFFFFF"
            horizontalAlignment: Text.AlignHCenter
        }
        
        // Subtitle
        Text {
            Layout.fillWidth: true
            text: "Chọn chế độ chơi"
            font.family: "Lexend"
            font.pixelSize: 14
            color: Qt.rgba(1.0, 1.0, 1.0, 0.9)
            horizontalAlignment: Text.AlignHCenter
        }
        
        Item { Layout.preferredHeight: 20 }
        
        // QuickMode Button
        Button {
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            text: "⚡ QuickMode\nChế độ nhanh - 15 câu hỏi"
            font.family: "Lexend"
            font.pixelSize: 16
            font.bold: true
            
            background: Rectangle {
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#4CAF50" }
                    GradientStop { position: 1.0; color: "#45a049" }
                }
                radius: 15
            }
            
            contentItem: Text {
                text: parent.text
                font: parent.font
                color: "#FFFFFF"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            
            onClicked: {
                if (stackView) {
                    stackView.push("QuickModeGame.qml", {
                        "stackView": stackView,
                        "username": username
                    })
                }
            }
        }
        
        Item { Layout.preferredHeight: 15 }
        
        // 1vN Mode Button
        Button {
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            text: "⚔️ 1vN Mode\nChế độ đối kháng"
            font.family: "Lexend"
            font.pixelSize: 16
            font.bold: true
            
            background: Rectangle {
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#FF6B6B" }
                    GradientStop { position: 1.0; color: "#ee5a6f" }
                }
                radius: 15
            }
            
            contentItem: Text {
                text: parent.text
                font: parent.font
                color: "#FFFFFF"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            
            onClicked: {
                if (stackView) {
                    stackView.push("OneVNMode.qml", {
                        "stackView": stackView,
                        "username": username
                    })
                }
            }
        }
        
        Item { Layout.fillHeight: true }
        }
        
        // Right side: Friends list
        Rectangle {
            Layout.preferredWidth: 250
            Layout.fillHeight: true
            radius: 15
            color: Qt.rgba(1.0, 1.0, 1.0, 0.95)
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 10
                
                // Header - separate from ScrollView to avoid overlap
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40  // Fixed height for header
                    color: "transparent"
                    z: 100  // Very high z-index to ensure it's above everything
                    
                    RowLayout {
                        anchors.fill: parent
                        spacing: 10
                        
                        Text {
                            Layout.fillWidth: true
                            text: "Bạn bè"
                            font.family: "Lexend"
                            font.pixelSize: 18
                            font.bold: true
                            color: "#333333"
                        }
                        
                        Button {
                            text: "➕"
                            font.pixelSize: 16
                            flat: true
                            enabled: true
                            Layout.preferredWidth: 35
                            Layout.preferredHeight: 35
                            
                            background: Rectangle {
                                color: parent.hovered ? Qt.rgba(0.0, 0.0, 0.0, 0.1) : "transparent"
                                radius: 4
                            }
                            
                            onClicked: {
                                console.log("➕ button clicked")
                                console.log("stackView:", stackView)
                                if (stackView) {
                                    stackView.push("FriendsList.qml", {
                                        "stackView": stackView,
                                        "username": username
                                    })
                                } else {
                                    console.log("ERROR: stackView is null!")
                                }
                            }
                        }
                    }
                }
                
                // Friends list
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    z: 0  // Ensure ScrollView is below header buttons
                    
                    ListView {
                        id: friendsListView
                        width: parent.width
                        model: friendsList
                        spacing: 5
                        
                        delegate: Rectangle {
                            width: friendsListView.width
                            height: 50
                            color: (modelData.online || modelData.onlineStatus === "online" || modelData.onlineStatus === "in_game") ? "#E8F5E9" : "#F5F5F5"
                            radius: 8
                            
                            property bool isOnline: modelData.online || modelData.onlineStatus === "online" || modelData.onlineStatus === "in_game"
                            
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 10
                                
                                // Online indicator
                                Rectangle {
                                    Layout.preferredWidth: 10
                                    Layout.preferredHeight: 10
                                    radius: 5
                                    color: parent.parent.isOnline ? "#4CAF50" : "#CCCCCC"
                                }
                                
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.username || ""
                                    font.family: "Lexend"
                                    font.pixelSize: 14
                                    color: "#333333"
                                    elide: Text.ElideRight
                                }
                                
                                // Unread message badge "!"
                                Rectangle {
                                    Layout.preferredWidth: 20
                                    Layout.preferredHeight: 20
                                    Layout.alignment: Qt.AlignVCenter
                                    radius: 10
                                    color: "#FF5252"
                                    visible: modelData.hasUnreadMessages || false
                                    
                                    // Disable mouse events so clicks pass through to parent
                                    enabled: false
                                    
                                    Text {
                                        anchors.centerIn: parent
                                        text: "!"
                                        font.family: "Lexend"
                                        font.pixelSize: 12
                                        font.bold: true
                                        color: "#FFFFFF"
                                    }
                                }
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    var friendId = modelData.id || modelData.userId || 0
                                    var friendName = modelData.username || ""
                                    
                                    // Reset unread badge immediately (before push to avoid delay)
                                    for (var i = 0; i < friendsList.length; i++) {
                                        if (friendsList[i].id === friendId || friendsList[i].userId === friendId) {
                                            friendsList[i].hasUnreadMessages = false
                                            var newList = friendsList.slice()
                                            friendsList = newList
                                            break
                                        }
                                    }
                                    
                                    // Push ChatScreen
                                    if (stackView) {
                                        stackView.push("ChatScreen.qml", {
                                            "stackView": stackView,
                                            "username": username,
                                            "friendId": friendId,
                                            "friendName": friendName
                                        })
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

