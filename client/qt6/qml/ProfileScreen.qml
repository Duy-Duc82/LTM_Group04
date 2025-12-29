import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: profileScreen
    width: parent ? parent.width : 360
    height: parent ? parent.height : 640
    
    property StackView stackView
    property string username: ""
    property int userId: 0
    
    // Profile data from server
    property var profileData: ({
        "user_id": 0,
        "username": "",
        "avatar_img": "",
        "quickmode_games": 0,
        "onevn_games": 0,
        "quickmode_wins": 0,
        "onevn_wins": 0
    })
    
    // Loading and error states
    property bool isLoading: false
    property string errorMessage: ""
    
    // Avatar image source
    property string avatarSource: {
        if (profileData.avatar_img && profileData.avatar_img !== "") {
            var imgPath = profileData.avatar_img
            // Convert file path to file:// URL format
            if (!imgPath.startsWith("file://") && !imgPath.startsWith("http://") && !imgPath.startsWith("https://") && !imgPath.startsWith("qrc://")) {
                // Windows path: C:/path -> file:///C:/path
                // Unix path: /path -> file:///path
                // Replace backslashes with forward slashes
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
        return "" // Will show initial letter instead
    }
    
    // Background color
    Rectangle {
        anchors.fill: parent
        color: "#2E1A47"
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        anchors.topMargin: 24
        anchors.bottomMargin: 24
        spacing: 0
        
        // Top Bar with back button
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            spacing: 16
            
            // Back button
            Rectangle {
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                color: "transparent"
                
                Text {
                    anchors.centerIn: parent
                    text: "←"
                    font.pixelSize: 24
                    color: "#FFFFFF"
                }
                
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (stackView) {
                            stackView.pop()
                        }
                    }
                }
            }
            
            // Title
            Text {
                Layout.fillWidth: true
                text: "👤 Hồ Sơ"
                font.family: "Lexend"
                font.pixelSize: 20
                font.bold: true
                color: "#FFFFFF"
            }
            
            // Refresh button
            Rectangle {
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                color: "transparent"
                
                Text {
                    anchors.centerIn: parent
                    text: "🔄"
                    font.pixelSize: 20
                    color: isLoading ? "#B0B0B0" : "#FFFFFF"
                }
                
                MouseArea {
                    anchors.fill: parent
                    enabled: !isLoading
                    onClicked: {
                        isLoading = true
                        errorMessage = ""
                        networkClient.sendGetProfile()
                    }
                }
            }
        }
        
        // Spacing
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 24
        }
        
        // Loading indicator
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "transparent"
            visible: isLoading && !errorMessage
            
            Column {
                anchors.centerIn: parent
                spacing: 16
                
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "⏳"
                    font.pixelSize: 48
                }
                
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Đang tải..."
                    font.family: "Lexend"
                    font.pixelSize: 16
                    color: "#B0B0B0"
                }
            }
        }
        
        // Error message
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "transparent"
            visible: errorMessage !== ""
            
            Column {
                anchors.centerIn: parent
                spacing: 16
                
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "❌"
                    font.pixelSize: 48
                }
                
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: errorMessage
                    font.family: "Lexend"
                    font.pixelSize: 16
                    color: "#FF6B6B"
                    width: parent.parent.width - 48
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                }
                
                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 120
                    height: 40
                    radius: 20
                    color: "#5D4586"
                    
                    Text {
                        anchors.centerIn: parent
                        text: "Thử lại"
                        font.family: "Lexend"
                        font.pixelSize: 14
                        color: "#FFFFFF"
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            isLoading = true
                            errorMessage = ""
                            networkClient.sendGetProfile()
                        }
                    }
                }
            }
        }
        
        // Profile content
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            visible: !isLoading && errorMessage === ""
            
            Column {
                width: parent.width
                spacing: 24
                
                // Avatar and username section
                Rectangle {
                    width: parent.width
                    height: 140
                    radius: 16
                    color: "#3D2B56"
                    border.color: "#5D4586"
                    border.width: 1
                    
                    Column {
                        anchors.centerIn: parent
                        spacing: 12
                        
                        // Avatar (clickable to change)
                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: 80
                            height: 80
                            radius: 40
                            color: "#5D4586"
                            clip: true
                            
                            // Avatar image (will be clipped to circle by parent Rectangle)
                            Image {
                                id: avatarImage
                                anchors.fill: parent
                                source: avatarSource
                                fillMode: Image.PreserveAspectCrop
                                visible: avatarSource !== ""
                            }
                            
                            Text {
                                anchors.centerIn: parent
                                text: profileData.username ? profileData.username.charAt(0).toUpperCase() : "U"
                                font.family: "Lexend"
                                font.pixelSize: 36
                                font.bold: true
                                color: "#FFFFFF"
                                visible: avatarSource === ""
                            }
                            
                            // Edit icon overlay
                            Rectangle {
                                anchors.bottom: parent.bottom
                                anchors.right: parent.right
                                width: 24
                                height: 24
                                radius: 12
                                color: "#FFC107"
                                border.color: "#FFFFFF"
                                border.width: 2
                                
                                Text {
                                    anchors.centerIn: parent
                                    text: "✏"
                                    font.pixelSize: 14
                                    color: "#FFFFFF"
                                }
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    var filePath = fileDialogHelper.openImageFile()
                                    if (filePath !== "") {
                                        console.log("Selected avatar file:", filePath)
                                        // Convert to file:// URL format for QML Image
                                        var urlPath = filePath
                                        if (!urlPath.startsWith("file://")) {
                                            // Windows path: C:/path -> file:///C:/path
                                            urlPath = "file:///" + urlPath.replace(/\\/g, "/")
                                        }
                                        // Update local display immediately
                                        profileData.avatar_img = filePath
                                        avatarSource = urlPath
                                        // Send to server to save
                                        networkClient.sendUpdateAvatar(filePath)
                                    }
                                }
                            }
                        }
                        
                        // Username
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: profileData.username || username || "User"
                            font.family: "Lexend"
                            font.pixelSize: 20
                            font.bold: true
                            color: "#FFFFFF"
                        }
                    }
                }
                
                // Statistics section
                Rectangle {
                    width: parent.width
                    radius: 16
                    color: "#3D2B56"
                    border.color: "#5D4586"
                    border.width: 1
                    
                    Column {
                        id: statsColumn
                        width: parent.width - 40
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.topMargin: 20
                        anchors.leftMargin: 20
                        spacing: 16
                        
                        // Section title
                        Text {
                            text: "📊 Thống Kê"
                            font.family: "Lexend"
                            font.pixelSize: 18
                            font.bold: true
                            color: "#FFFFFF"
                        }
                        
                        // QuickMode stats
                        Rectangle {
                            width: parent.width
                            height: 100
                            radius: 12
                            color: "#2E1A47"
                            
                            Column {
                                anchors.fill: parent
                                anchors.margins: 16
                                spacing: 8
                                
                                Text {
                                    text: "⚡ Quick Mode"
                                    font.family: "Lexend"
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "#FFC107"
                                }
                                
                                Row {
                                    width: parent.width
                                    spacing: 20
                                    
                                    Column {
                                        spacing: 4
                                        
                                        Text {
                                            text: "Số lần chơi"
                                            font.family: "Lexend"
                                            font.pixelSize: 12
                                            color: "#B0B0B0"
                                        }
                                        
                                        Text {
                                            text: {
                                                var games = profileData.quickmode_games
                                                if (games === undefined || games === null) return "0"
                                                return Number(games).toString()
                                            }
                                            font.family: "Lexend"
                                            font.pixelSize: 24
                                            font.bold: true
                                            color: "#FFFFFF"
                                        }
                                    }
                                    
                                    Item {
                                        width: 1
                                        height: parent.height
                                        Rectangle {
                                            anchors.centerIn: parent
                                            width: 1
                                            height: parent.height - 8
                                            color: "#5D4586"
                                        }
                                    }
                                    
                                    Column {
                                        spacing: 4
                                        
                                        Text {
                                            text: "Số lần thắng"
                                            font.family: "Lexend"
                                            font.pixelSize: 12
                                            color: "#B0B0B0"
                                        }
                                        
                                        Text {
                                            text: {
                                                var wins = profileData.quickmode_wins
                                                if (wins === undefined || wins === null) return "0"
                                                return Number(wins).toString()
                                            }
                                            font.family: "Lexend"
                                            font.pixelSize: 24
                                            font.bold: true
                                            color: "#4CAF50"
                                        }
                                    }
                                    
                                    Item {
                                        width: 1
                                        height: parent.height
                                        Rectangle {
                                            anchors.centerIn: parent
                                            width: 1
                                            height: parent.height - 8
                                            color: "#5D4586"
                                        }
                                    }
                                    
                                    Column {
                                        spacing: 4
                                        
                                        Text {
                                            text: "Tỷ lệ thắng"
                                            font.family: "Lexend"
                                            font.pixelSize: 12
                                            color: "#B0B0B0"
                                        }
                                        
                                        Text {
                                            text: {
                                                var games = profileData.quickmode_games
                                                var wins = profileData.quickmode_wins
                                                if (games === undefined || games === null) games = 0
                                                if (wins === undefined || wins === null) wins = 0
                                                if (Number(games) === 0) return "0%"
                                                var rate = Math.round((Number(wins) / Number(games)) * 100)
                                                return rate + "%"
                                            }
                                            font.family: "Lexend"
                                            font.pixelSize: 24
                                            font.bold: true
                                            color: "#FFC107"
                                        }
                                    }
                                }
                            }
                        }
                        
                        // 1vN stats
                        Rectangle {
                            width: parent.width
                            height: 100
                            radius: 12
                            color: "#2E1A47"
                            
                            Column {
                                anchors.fill: parent
                                anchors.margins: 16
                                spacing: 8
                                
                                Text {
                                    text: "⚔️ 1vN Mode"
                                    font.family: "Lexend"
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "#FF6B6B"
                                }
                                
                                Row {
                                    width: parent.width
                                    spacing: 20
                                    
                                    Column {
                                        spacing: 4
                                        
                                        Text {
                                            text: "Số lần chơi"
                                            font.family: "Lexend"
                                            font.pixelSize: 12
                                            color: "#B0B0B0"
                                        }
                                        
                                        Text {
                                            text: {
                                                var games = profileData.onevn_games
                                                if (games === undefined || games === null) return "0"
                                                return Number(games).toString()
                                            }
                                            font.family: "Lexend"
                                            font.pixelSize: 24
                                            font.bold: true
                                            color: "#FFFFFF"
                                        }
                                    }
                                    
                                    Item {
                                        width: 1
                                        height: parent.height
                                        Rectangle {
                                            anchors.centerIn: parent
                                            width: 1
                                            height: parent.height - 8
                                            color: "#5D4586"
                                        }
                                    }
                                    
                                    Column {
                                        spacing: 4
                                        
                                        Text {
                                            text: "Số lần thắng"
                                            font.family: "Lexend"
                                            font.pixelSize: 12
                                            color: "#B0B0B0"
                                        }
                                        
                                        Text {
                                            text: {
                                                var wins = profileData.onevn_wins
                                                if (wins === undefined || wins === null) return "0"
                                                return Number(wins).toString()
                                            }
                                            font.family: "Lexend"
                                            font.pixelSize: 24
                                            font.bold: true
                                            color: "#4CAF50"
                                        }
                                    }
                                    
                                    Item {
                                        width: 1
                                        height: parent.height
                                        Rectangle {
                                            anchors.centerIn: parent
                                            width: 1
                                            height: parent.height - 8
                                            color: "#5D4586"
                                        }
                                    }
                                    
                                    Column {
                                        spacing: 4
                                        
                                        Text {
                                            text: "Tỷ lệ thắng"
                                            font.family: "Lexend"
                                            font.pixelSize: 12
                                            color: "#B0B0B0"
                                        }
                                        
                                        Text {
                                            text: {
                                                var games = profileData.onevn_games
                                                var wins = profileData.onevn_wins
                                                if (games === undefined || games === null) games = 0
                                                if (wins === undefined || wins === null) wins = 0
                                                if (Number(games) === 0) return "0%"
                                                var rate = Math.round((Number(wins) / Number(games)) * 100)
                                                return rate + "%"
                                            }
                                            font.family: "Lexend"
                                            font.pixelSize: 24
                                            font.bold: true
                                            color: "#FF6B6B"
                                        }
                                    }
                                }
                            }
                        }
                        
                        // Total stats
                        Rectangle {
                            width: parent.width
                            height: 60
                            radius: 12
                            color: "#2E1A47"
                            
                            Row {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 16
                                
                                Column {
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 2
                                    
                                    Text {
                                        text: "Tổng số trận"
                                        font.family: "Lexend"
                                        font.pixelSize: 11
                                        color: "#B0B0B0"
                                    }
                                    
                                    Text {
                                        text: ((profileData.quickmode_games || 0) + (profileData.onevn_games || 0)).toString()
                                        font.family: "Lexend"
                                        font.pixelSize: 20
                                        font.bold: true
                                        color: "#FFFFFF"
                                    }
                                }
                                
                                Item {
                                    width: 1
                                    height: parent.height
                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: 1
                                        height: parent.height
                                        color: "#5D4586"
                                    }
                                }
                                
                                Column {
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 2
                                    
                                    Text {
                                        text: "Tổng số thắng"
                                        font.family: "Lexend"
                                        font.pixelSize: 11
                                        color: "#B0B0B0"
                                    }
                                    
                                    Text {
                                        text: ((profileData.quickmode_wins || 0) + (profileData.onevn_wins || 0)).toString()
                                        font.family: "Lexend"
                                        font.pixelSize: 20
                                        font.bold: true
                                        color: "#4CAF50"
                                    }
                                }
                            }
                        }
                    }
                    
                    // Set height based on content
                    height: statsColumn.childrenRect.height + 40
                }
            }
        }
    }
    
    // Connect to NetworkClient signals
    Connections {
        target: networkClient
        
        function onProfileReceived(profile) {
            console.log("=== Profile received ===")
            console.log("Profile data:", JSON.stringify(profile))
            isLoading = false
            errorMessage = ""
            profileData = profile
        }
        
        function onErrorOccurred(error) {
            console.log("Profile error:", error)
            isLoading = false
            errorMessage = error || "Không thể tải thông tin profile"
        }
        
        function onAvatarUpdated(success, message) {
            console.log("Avatar update result:", success, message)
            if (success) {
                // Profile will be refreshed automatically by NetworkClient
                console.log("Avatar đã được cập nhật thành công")
            } else {
                console.log("Lỗi cập nhật avatar:", message)
            }
        }
    }
    
    Component.onCompleted: {
        // Request profile data from server
        isLoading = true
        errorMessage = ""
        networkClient.sendGetProfile()
    }
}
