import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: leaderboardScreen
    width: parent ? parent.width : 360
    height: parent ? parent.height : 640
    
    property StackView stackView
    property string username: ""
    property int currentUserId: networkClient ? networkClient.getUserId() : 0
    
    // Leaderboard data from server
    property var leaderboardData: []
    
    // Loading and error states
    property bool isLoading: false
    property string errorMessage: ""
    
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
                text: "🏆 Bảng Xếp Hạng"
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
                        networkClient.sendLeaderboard()
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
                            networkClient.sendLeaderboard()
                        }
                    }
                }
            }
        }
        
        // Leaderboard content
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            visible: !isLoading && errorMessage === ""
            
            Column {
                width: parent.width
                spacing: 12
                
                // Top 3 podium
                Row {
                    width: parent.width
                    height: 120
                    spacing: 8
                    
                    // 2nd place (silver)
                    Rectangle {
                        width: (parent.width - parent.spacing * 2) / 3
                        height: 100
                        anchors.bottom: parent.bottom
                        radius: 16
                        color: leaderboardData.length > 1 && leaderboardData[1].user_id === currentUserId ? "#4A3A6B" : "#3D2B56"
                        border.color: leaderboardData.length > 1 && leaderboardData[1].user_id === currentUserId ? "#FFC107" : "#C0C0C0"
                        border.width: leaderboardData.length > 1 && leaderboardData[1].user_id === currentUserId ? 3 : 2
                        
                        Column {
                            anchors.centerIn: parent
                            spacing: 4
                            
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "🥈"
                                font.pixelSize: 24
                            }
                            
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: leaderboardData.length > 1 ? (leaderboardData[1].username || "User 2") : "---"
                                font.family: "Lexend"
                                font.pixelSize: 12
                                font.bold: true
                                color: "#FFFFFF"
                                elide: Text.ElideRight
                                width: parent.parent.width - 16
                            }
                            
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: leaderboardData.length > 1 ? (leaderboardData[1].total_wins || 0) + " trận thắng" : "0"
                                font.family: "Lexend"
                                font.pixelSize: 10
                                color: "#B0B0B0"
                            }
                        }
                    }
                    
                    // 1st place (gold) - taller
                    Rectangle {
                        width: (parent.width - parent.spacing * 2) / 3
                        height: 120
                        anchors.bottom: parent.bottom
                        radius: 16
                        color: leaderboardData.length > 0 && leaderboardData[0].user_id === currentUserId ? "#4A3A6B" : "#3D2B56"
                        border.color: leaderboardData.length > 0 && leaderboardData[0].user_id === currentUserId ? "#FFC107" : "#FFC107"
                        border.width: leaderboardData.length > 0 && leaderboardData[0].user_id === currentUserId ? 4 : 3
                        
                        Column {
                            anchors.centerIn: parent
                            spacing: 4
                            
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "🥇"
                                font.pixelSize: 32
                            }
                            
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: leaderboardData.length > 0 ? (leaderboardData[0].username || "User 1") : "---"
                                font.family: "Lexend"
                                font.pixelSize: 14
                                font.bold: true
                                color: "#FFC107"
                                elide: Text.ElideRight
                                width: parent.parent.width - 16
                            }
                            
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: leaderboardData.length > 0 ? (leaderboardData[0].total_wins || 0) + " trận thắng" : "0"
                                font.family: "Lexend"
                                font.pixelSize: 11
                                color: "#B0B0B0"
                            }
                        }
                    }
                    
                    // 3rd place (bronze)
                    Rectangle {
                        width: (parent.width - parent.spacing * 2) / 3
                        height: 80
                        anchors.bottom: parent.bottom
                        radius: 16
                        color: leaderboardData.length > 2 && leaderboardData[2].user_id === currentUserId ? "#4A3A6B" : "#3D2B56"
                        border.color: leaderboardData.length > 2 && leaderboardData[2].user_id === currentUserId ? "#FFC107" : "#CD7F32"
                        border.width: leaderboardData.length > 2 && leaderboardData[2].user_id === currentUserId ? 3 : 2
                        
                        Column {
                            anchors.centerIn: parent
                            spacing: 4
                            
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "🥉"
                                font.pixelSize: 20
                            }
                            
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: leaderboardData.length > 2 ? (leaderboardData[2].username || "User 3") : "---"
                                font.family: "Lexend"
                                font.pixelSize: 11
                                font.bold: true
                                color: "#FFFFFF"
                                elide: Text.ElideRight
                                width: parent.parent.width - 16
                            }
                            
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: leaderboardData.length > 2 ? (leaderboardData[2].total_wins || 0) + " trận thắng" : "0"
                                font.family: "Lexend"
                                font.pixelSize: 9
                                color: "#B0B0B0"
                            }
                        }
                    }
                }
                
                // Rest of leaderboard list (starting from rank 4)
                Repeater {
                    model: leaderboardData.length > 3 ? leaderboardData.length - 3 : 0
                    delegate: Rectangle {
                        width: parent.width
                        height: 64
                        radius: 12
                        color: leaderboardData[index + 3].user_id === currentUserId ? "#4A3A6B" : "#3D2B56"
                        border.color: leaderboardData[index + 3].user_id === currentUserId ? "#FFC107" : "#5D4586"
                        border.width: leaderboardData[index + 3].user_id === currentUserId ? 2 : 1
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 12
                            
                            // Rank number
                            Text {
                                Layout.preferredWidth: 32
                                text: (index + 4).toString()
                                font.family: "Lexend"
                                font.pixelSize: 16
                                font.bold: true
                                color: "#B0B0B0"
                                horizontalAlignment: Text.AlignHCenter
                            }
                            
                            // Avatar placeholder
                            Rectangle {
                                Layout.preferredWidth: 36
                                Layout.preferredHeight: 36
                                radius: 18
                                color: "#5D4586"
                                
                                Text {
                                    anchors.centerIn: parent
                                    text: leaderboardData[index + 3].username ? leaderboardData[index + 3].username.charAt(0).toUpperCase() : "U"
                                    font.family: "Lexend"
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "#FFFFFF"
                                }
                            }
                            
                            // Username
                            Text {
                                Layout.fillWidth: true
                                text: leaderboardData[index + 3].username || "User " + (index + 4)
                                font.family: "Lexend"
                                font.pixelSize: 14
                                color: "#FFFFFF"
                                elide: Text.ElideRight
                            }
                            
                            // Wins count
                            Text {
                                Layout.preferredWidth: 80
                                text: (leaderboardData[index + 3].total_wins || 0) + " trận"
                                font.family: "Lexend"
                                font.pixelSize: 12
                                color: "#FFC107"
                                horizontalAlignment: Text.AlignRight
                            }
                        }
                    }
                }
                
                // Empty state
                Rectangle {
                    width: parent.width
                    height: 200
                    color: "transparent"
                    visible: leaderboardData.length === 0
                    
                    Column {
                        anchors.centerIn: parent
                        spacing: 16
                        
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "📊"
                            font.pixelSize: 48
                        }
                        
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "Chưa có dữ liệu"
                            font.family: "Lexend"
                            font.pixelSize: 16
                            color: "#B0B0B0"
                        }
                    }
                }
            }
        }
    }
    
    // Connect to NetworkClient signals
    Connections {
        target: networkClient
        
        function onLeaderboardReceived(leaderboard) {
            console.log("=== Leaderboard received ===")
            console.log("Leaderboard count:", leaderboard.length)
            console.log("Leaderboard data:", JSON.stringify(leaderboard))
            
            isLoading = false
            errorMessage = ""
            
            var tempList = []
            for (var i = 0; i < leaderboard.length; i++) {
                var player = leaderboard[i]
                tempList.push({
                    user_id: player.user_id || player.userId || 0,
                    username: player.username || "",
                    total_wins: player.total_wins || 0
                })
            }
            leaderboardData = tempList
        }
        
        function onErrorOccurred(error) {
            console.log("Leaderboard error:", error)
            isLoading = false
            errorMessage = error || "Không thể tải bảng xếp hạng"
        }
    }
    
    Component.onCompleted: {
        // Request leaderboard data from server
        isLoading = true
        errorMessage = ""
        currentUserId = networkClient ? networkClient.getUserId() : 0
        networkClient.sendLeaderboard()
    }
}
