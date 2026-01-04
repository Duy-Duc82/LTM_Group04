import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: historyScreen
    width: parent ? parent.width : 360
    height: parent ? parent.height : 640
    
    property StackView stackView
    property int userId: 0
    
    // Game history data
    property var gameHistory: []
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
                text: "📜 Lịch Sử Game 1vN"
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
                        networkClient.sendGetOneVNHistory()
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
            visible: isLoading && !errorMessage && gameHistory.length === 0
            
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
                    text: "Đang tải lịch sử..."
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
            visible: errorMessage !== "" && gameHistory.length === 0
            
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
                            networkClient.sendGetOneVNHistory()
                        }
                    }
                }
            }
        }
        
        // Empty state
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "transparent"
            visible: !isLoading && errorMessage === "" && gameHistory.length === 0
            
            Column {
                anchors.centerIn: parent
                spacing: 16
                
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "📭"
                    font.pixelSize: 64
                }
                
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Chưa có lịch sử game"
                    font.family: "Lexend"
                    font.pixelSize: 18
                    font.bold: true
                    color: "#FFFFFF"
                }
                
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Bạn chưa chơi game 1vN nào"
                    font.family: "Lexend"
                    font.pixelSize: 14
                    color: "#B0B0B0"
                    width: parent.parent.width - 48
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
        
        // Game history list
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            visible: !isLoading && errorMessage === "" && gameHistory.length > 0
            
            Column {
                width: parent.width
                spacing: 12
                
                Repeater {
                    model: gameHistory
                    
                    Rectangle {
                        width: parent.width
                        height: 120
                        radius: 16
                        color: "#3D2B56"
                        border.color: "#5D4586"
                        border.width: 1
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 16
                            
                            // Game icon
                            Rectangle {
                                Layout.preferredWidth: 64
                                Layout.preferredHeight: 64
                                radius: 12
                                color: "#5D4586"
                                
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.is_winner ? "🏆" : "⚔️"
                                    font.pixelSize: 32
                                }
                            }
                            
                            // Game info
                            Column {
                                Layout.fillWidth: true
                                spacing: 8
                                
                                Text {
                                    text: {
                                        var date = new Date(modelData.played_at)
                                        var dateStr = date.toLocaleDateString("vi-VN")
                                        var timeStr = date.toLocaleTimeString("vi-VN", {hour: '2-digit', minute: '2-digit'})
                                        return dateStr + " " + timeStr
                                    }
                                    font.family: "Lexend"
                                    font.pixelSize: 14
                                    font.bold: true
                                    color: "#FFFFFF"
                                }
                                
                                Row {
                                    spacing: 12
                                    
                                    Text {
                                        text: "Điểm: " + (modelData.final_score || 0)
                                        font.family: "Lexend"
                                        font.pixelSize: 12
                                        color: "#FFC107"
                                    }
                                    
                                    Text {
                                        text: "•"
                                        font.family: "Lexend"
                                        font.pixelSize: 12
                                        color: "#5D4586"
                                    }
                                    
                                    Text {
                                        text: "Hạng: #" + (modelData.final_rank || "-")
                                        font.family: "Lexend"
                                        font.pixelSize: 12
                                        color: "#B0B0B0"
                                    }
                                }
                                
                                Text {
                                    text: modelData.is_winner ? "✓ Chiến thắng" : "✗ Thua"
                                    font.family: "Lexend"
                                    font.pixelSize: 12
                                    color: modelData.is_winner ? "#4CAF50" : "#FF6B6B"
                                }
                            }
                            
                            // Replay button
                            Rectangle {
                                Layout.preferredWidth: 80
                                Layout.preferredHeight: 40
                                radius: 20
                                color: "#FFC107"
                                
                                Text {
                                    anchors.centerIn: parent
                                    text: "Xem lại"
                                    font.family: "Lexend"
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: "#2E1A47"
                                }
                                
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        if (stackView) {
                                            stackView.push("OneVNReplayScreen.qml", {
                                                "stackView": stackView,
                                                "sessionId": modelData.session_id,
                                                "gameData": modelData
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
    
    // Connect to NetworkClient signals
    Connections {
        target: networkClient
        
        function onOneVNHistoryReceived(history) {
            console.log("=== OneVN History received ===")
            console.log("History:", JSON.stringify(history))
            isLoading = false
            errorMessage = ""
            gameHistory = history || []
        }
        
        function onErrorOccurred(error) {
            console.log("OneVN History error:", error)
            isLoading = false
            if (gameHistory.length === 0) {
                errorMessage = error || "Không thể tải lịch sử game"
            }
        }
    }
    
    Component.onCompleted: {
        // Request history data from server
        isLoading = true
        errorMessage = ""
        networkClient.sendGetOneVNHistory()
    }
}

