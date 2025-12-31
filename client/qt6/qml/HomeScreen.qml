import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: homeScreen
    width: parent ? parent.width : 360
    height: parent ? parent.height : 640
    
    property StackView stackView
    property string username: ""
    property int userId: 0
    property int points: 0
    property string rank: "Bronze"
    
    // Unread messages tracking
    property bool hasUnreadMessages: false
    
    // Avatar data
    property string avatarPath: ""
    property string avatarSource: {
        if (avatarPath && avatarPath !== "") {
            var imgPath = avatarPath
            // Convert file path to file:// URL format
            if (!imgPath.startsWith("file://") && !imgPath.startsWith("http://") && !imgPath.startsWith("https://") && !imgPath.startsWith("qrc://")) {
                imgPath = imgPath.replace(/\\/g, "/")
                if (imgPath.length > 1 && imgPath[1] === ':') {
                    // Windows absolute path (C:/...)
                    imgPath = "file:///" + imgPath
                } else if (imgPath.length > 0 && imgPath[0] === '/') {
                    // Unix absolute path (/...)
                    imgPath = "file://" + imgPath
                } else {
                    // Relative path
                    imgPath = "file:///" + imgPath
                }
            }
            return imgPath
        }
        return ""
    }
    
    // Background color
    Rectangle {
        anchors.fill: parent
        color: "#2E1A47"
    }
    
    ColumnLayout {
        id: mainColumn
        anchors.fill: parent
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        anchors.topMargin: 24
        anchors.bottomMargin: 24
        spacing: 0
        
        // Top Bar (56px)
        Rectangle {
            id: topBar
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: "transparent"
            
            RowLayout {
                anchors.fill: parent
                spacing: 12
                
                // Avatar (36x36)
                Rectangle {
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                    radius: 18
                    color: "#5D4586"
                    clip: true
                    
                    // Avatar image
                    Image {
                        anchors.fill: parent
                        source: avatarSource
                        fillMode: Image.PreserveAspectCrop
                        visible: avatarSource !== ""
                    }
                    
                    // Initial letter fallback
                    Text {
                        anchors.centerIn: parent
                        text: username.length > 0 ? username.charAt(0).toUpperCase() : "U"
                        font.family: "Lexend"
                        font.pixelSize: 18
                        font.bold: true
                        color: "#FFFFFF"
                        visible: avatarSource === ""
                    }
                }
                
                // Username and Points/Rank
                Column {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 2
                    
                    Text {
                        text: username
                        font.family: "Lexend"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#FFFFFF"
                    }
                    
                    Text {
                        text: points + " điểm • " + rank
                        font.family: "Lexend"
                        font.pixelSize: 12
                        color: "#B0B0B0"
                    }
                }
                
                // Notifications icon
                Rectangle {
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                    color: "transparent"
                    
                    Text {
                        anchors.centerIn: parent
                        text: "🔔"
                        font.pixelSize: 20
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            // TODO: Navigate to notifications
                            console.log("Notifications clicked")
                        }
                    }
                }
                
                // Logout button
                Rectangle {
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                    color: "transparent"
                    
                    Text {
                        anchors.centerIn: parent
                        text: "🚪"
                        font.pixelSize: 20
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            // Call logout directly
                            if (networkClient.isLoggedIn()) {
                                networkClient.sendLogout()
                            }
                        }
                    }
                }
            }
        }
        
        // Spacing after top bar: 24px
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 24
        }
        
        // Scrollable content area
        Item {
            id: contentArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            ScrollView {
                anchors.fill: parent
                clip: true
                
                Column {
                    width: parent.width
                    spacing: 16
                    
                    // Card 1: Quick Mode (312x168)
                    Rectangle {
                        id: quickModeCard
                        width: parent.width
                        height: 168
                        radius: 16
                        color: "#3D2B56"
                        border.color: "#5D4586"
                        border.width: 1
                        
                        Column {
                            anchors.fill: parent
                            anchors.margins: 20
                            spacing: 12
                            
                            Text {
                                text: "⚡ Quick Mode"
                                font.family: "Lexend"
                                font.pixelSize: 20
                                font.bold: true
                                color: "#FFFFFF"
                            }
                            
                            Text {
                                width: parent.width
                                text: "Chế độ nhanh - 15 câu hỏi"
                                font.family: "Lexend"
                                font.pixelSize: 14
                                color: "#B0B0B0"
                                wrapMode: Text.WordWrap
                            }
                            
                            Item {
                                width: parent.width
                                height: 1
                            }
                            
                            // Primary Button "Chơi ngay" (48px)
                            Button {
                                width: parent.width
                                height: 48
                                text: "Chơi ngay"
                                font.family: "Lexend"
                                font.pixelSize: 16
                                font.bold: true
                                
                                background: Rectangle {
                                    color: "#FFC107"
                                    radius: 12
                                }
                                
                                contentItem: Text {
                                    text: parent.text
                                    font: parent.font
                                    color: "#000000"
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
                        }
                    }
                    
                    // Card 2: 1vN Mode (312x168)
                    Rectangle {
                        id: oneVNCard
                        width: parent.width
                        height: 168
                        radius: 16
                        color: "#3D2B56"
                        border.color: "#5D4586"
                        border.width: 1
                        
                        Column {
                            anchors.fill: parent
                            anchors.margins: 20
                            spacing: 12
                            
                            Text {
                                text: "⚔️ 1vN"
                                font.family: "Lexend"
                                font.pixelSize: 20
                                font.bold: true
                                color: "#FFFFFF"
                            }
                            
                            Text {
                                width: parent.width
                                text: "Chế độ đối kháng"
                                font.family: "Lexend"
                                font.pixelSize: 14
                                color: "#B0B0B0"
                                wrapMode: Text.WordWrap
                            }
                            
                            Item {
                                width: parent.width
                                height: 1
                            }
                            
                            // Two buttons row
                            Row {
                                width: parent.width
                                height: 48
                                spacing: 12
                                
                                // Secondary Button "Tham gia"
                                Button {
                                    width: (parent.width - parent.spacing) / 2
                                    height: 48
                                    text: "Tham gia"
                                    font.family: "Lexend"
                                    font.pixelSize: 16
                                    font.bold: true
                                    
                                    background: Rectangle {
                                        color: "#3D2B56"
                                        border.color: "#5D4586"
                                        border.width: 1
                                        radius: 12
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
                                
                                // Primary Button "Tạo phòng"
                                Button {
                                    width: (parent.width - parent.spacing) / 2
                                    height: 48
                                    text: "Tạo phòng"
                                    font.family: "Lexend"
                                    font.pixelSize: 16
                                    font.bold: true
                                    
                                    background: Rectangle {
                                        color: "#FFC107"
                                        radius: 12
                                    }
                                    
                                    contentItem: Text {
                                        text: parent.text
                                        font: parent.font
                                        color: "#000000"
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
                            }
                        }
                    }
                    
                    // Quick actions row (72px)
                    Rectangle {
                        width: parent.width
                        height: 72
                        color: "transparent"
                        
                        Row {
                            anchors.fill: parent
                            spacing: 12
                            
                            // Friends tile
                            Rectangle {
                                width: (parent.width - parent.spacing * 2) / 3
                                height: parent.height
                                radius: 12
                                color: "#3D2B56"
                                border.color: "#5D4586"
                                border.width: 1
                                
                                Column {
                                    anchors.centerIn: parent
                                    spacing: 4
                                    
                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: "👥"
                                        font.pixelSize: 32
                                    }
                                    
                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: "Friends"
                                        font.family: "Lexend"
                                        font.pixelSize: 12
                                        color: "#FFFFFF"
                                    }
                                }
                                
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        if (stackView) {
                                            stackView.push("FriendsList.qml", {
                                                "stackView": stackView,
                                                "username": username
                                            })
                                        }
                                    }
                                }
                            }
                            
                            // BXH (Leaderboard) tile
                            Rectangle {
                                width: (parent.width - parent.spacing * 2) / 3
                                height: parent.height
                                radius: 12
                                color: "#3D2B56"
                                border.color: "#5D4586"
                                border.width: 1
                                
                                Column {
                                    anchors.centerIn: parent
                                    spacing: 4
                                    
                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: "🏆"
                                        font.pixelSize: 32
                                    }
                                    
                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: "BXH"
                                        font.family: "Lexend"
                                        font.pixelSize: 12
                                        color: "#FFFFFF"
                                    }
                                }
                                
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        if (stackView) {
                                            stackView.push("LeaderboardScreen.qml", {
                                                "stackView": stackView,
                                                "username": username
                                            })
                                        }
                                    }
                                }
                            }
                            
                            // Chat tile
                            Rectangle {
                                width: (parent.width - parent.spacing * 2) / 3
                                height: parent.height
                                radius: 12
                                color: "#3D2B56"
                                border.color: "#5D4586"
                                border.width: 1
                                
                                Column {
                                    anchors.centerIn: parent
                                    spacing: 4
                                    
                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: "💬"
                                        font.pixelSize: 32
                                    }
                                    
                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: "Chat"
                                        font.family: "Lexend"
                                        font.pixelSize: 12
                                        color: "#FFFFFF"
                                    }
                                }
                                
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        // TODO: Navigate to chat
                                        console.log("Chat clicked")
                                    }
                                }
                            }
                        }
                    }
                    
                    // Mini leaderboard (~96px) - Optional
                    Rectangle {
                        width: parent.width
                        height: 96
                        radius: 16
                        color: "#3D2B56"
                        border.color: "#5D4586"
                        border.width: 1
                        
                        Column {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 8
                            
                            RowLayout {
                                width: parent.width
                                
                                Text {
                                    text: "🏆 Top 3"
                                    font.family: "Lexend"
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "#FFFFFF"
                                }
                                
                                Item {
                                    Layout.fillWidth: true
                                }
                                
                                Text {
                                    text: "Xem tất cả →"
                                    font.family: "Lexend"
                                    font.pixelSize: 12
                                    color: "#FFC107"
                                    
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            if (stackView) {
                                                stackView.push("LeaderboardScreen.qml", {
                                                    "stackView": stackView,
                                                    "username": username
                                                })
                                            }
                                        }
                                    }
                                }
                            }
                            
                            // Placeholder for top 3
                            Row {
                                width: parent.width
                                spacing: 8
                                
                                Repeater {
                                    model: 3
                                    
                                    Rectangle {
                                        width: (parent.width - parent.spacing * 2) / 3
                                        height: 48
                                        radius: 8
                                        color: "#2E1A47"
                                        
                                        Column {
                                            anchors.centerIn: parent
                                            spacing: 2
                                            
                                            Text {
                                                anchors.horizontalCenter: parent.horizontalCenter
                                                text: index === 0 ? "🥇" : index === 1 ? "🥈" : "🥉"
                                                font.pixelSize: 16
                                            }
                                            
                                            Text {
                                                anchors.horizontalCenter: parent.horizontalCenter
                                                text: "User " + (index + 1)
                                                font.family: "Lexend"
                                                font.pixelSize: 10
                                                color: "#B0B0B0"
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
        
        // Bottom Navigation (64px, fixed)
        Rectangle {
            id: bottomNav
            Layout.fillWidth: true
            Layout.preferredHeight: 64
            color: "#3D2B56"
            
            Row {
                anchors.fill: parent
                anchors.margins: 8
                
                // Home tab (active)
                Rectangle {
                    width: parent.width / 4
                    height: parent.height
                    color: "transparent"
                    
                    Column {
                        anchors.centerIn: parent
                        spacing: 4
                        
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "🏠"
                            font.pixelSize: 24
                        }
                        
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "Home"
                            font.family: "Lexend"
                            font.pixelSize: 11
                            color: "#FFC107" // Active color
                        }
                    }
                }
                
                // Friends tab
                Rectangle {
                    width: parent.width / 4
                    height: parent.height
                    color: "transparent"
                    
                    Column {
                        anchors.centerIn: parent
                        spacing: 4
                        
                        Item {
                            width: 24
                            height: 24
                            anchors.horizontalCenter: parent.horizontalCenter
                            
                            Text {
                                anchors.centerIn: parent
                                text: "👥"
                                font.pixelSize: 24
                            }
                            
                            // Unread message badge
                            Rectangle {
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.rightMargin: -4
                                anchors.topMargin: -4
                                width: 12
                                height: 12
                                radius: 6
                                color: "#FF5252"
                                visible: hasUnreadMessages
                                
                                Text {
                                    anchors.centerIn: parent
                                    text: "!"
                                    font.family: "Lexend"
                                    font.pixelSize: 8
                                    font.bold: true
                                    color: "#FFFFFF"
                                }
                            }
                        }
                        
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "Friends"
                            font.family: "Lexend"
                            font.pixelSize: 11
                            color: "#B0B0B0"
                        }
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            // Reset unread badge when opening FriendsList
                            hasUnreadMessages = false
                            
                            if (stackView) {
                                stackView.push("FriendsList.qml", {
                                    "stackView": stackView,
                                    "username": username
                                })
                            }
                        }
                    }
                }
                
                // Leaderboard tab
                Rectangle {
                    width: parent.width / 4
                    height: parent.height
                    color: "transparent"
                    
                    Column {
                        anchors.centerIn: parent
                        spacing: 4
                        
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "🏆"
                            font.pixelSize: 24
                        }
                        
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "BXH"
                            font.family: "Lexend"
                            font.pixelSize: 11
                            color: "#B0B0B0"
                        }
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (stackView) {
                                stackView.push("LeaderboardScreen.qml", {
                                    "stackView": stackView,
                                    "username": username
                                })
                            }
                        }
                    }
                }
                
                // Profile tab
                Rectangle {
                    width: parent.width / 4
                    height: parent.height
                    color: "transparent"
                    
                    Column {
                        anchors.centerIn: parent
                        spacing: 4
                        
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "👤"
                            font.pixelSize: 24
                        }
                        
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "Profile"
                            font.family: "Lexend"
                            font.pixelSize: 11
                            color: "#B0B0B0"
                        }
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (stackView) {
                                stackView.push("ProfileScreen.qml", {
                                    "stackView": stackView,
                                    "username": username,
                                    "userId": userId
                                })
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Connect to NetworkClient signals
    Connections {
        target: networkClient
        
        function onProfileReceived(profile) {
            // Update avatar when profile is received
            if (profile.avatar_img && profile.avatar_img !== "") {
                avatarPath = profile.avatar_img
            }
        }
        
        function onDmReceived(fromUserId, fromUsername, message, timestamp) {
            console.log("=== HomeScreen: dmReceived ===")
            console.log("From:", fromUserId, "Message:", message)
            
            // Set unread messages flag for footer badge
            hasUnreadMessages = true
        }
        
        function onLogoutResponse(success) {
            console.log("Logout response:", success)
            if (success) {
                // Navigate back to Signin screen
                // Socket will be disconnected by NetworkClient after logout
                if (stackView) {
                    // Clear stack and go to Signin
                    stackView.clear()
                    stackView.push("Signin.qml", {"stackView": stackView})
                }
            }
        }
    }
    
    Component.onCompleted: {
        // Request profile to get avatar
        if (userId > 0) {
            networkClient.sendGetProfile()
        }
    }
}