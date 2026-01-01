import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: replayScreen
    width: parent ? parent.width : 360
    height: parent ? parent.height : 640
    
    property StackView stackView
    property qint64 sessionId: 0
    property var replayData: ({})
    property var roundsList: []
    property var playersList: []
    property bool isLoading: false
    
    // Connect to NetworkClient
    Connections {
        target: networkClient
        
        function onReplayDetailsReceived(data) {
            console.log("=== Replay details received ===")
            console.log(JSON.stringify(data))
            replayData = data
            isLoading = false
            
            // Parse rounds
            if (data.rounds && Array.isArray(data.rounds)) {
                var tempRounds = []
                for (var i = 0; i < data.rounds.length; i++) {
                    tempRounds.push(data.rounds[i])
                }
                roundsList = tempRounds
            } else {
                roundsList = []
            }
            
            // Parse players
            if (data.players && Array.isArray(data.players)) {
                var tempPlayers = []
                for (var j = 0; j < data.players.length; j++) {
                    tempPlayers.push(data.players[j])
                }
                playersList = tempPlayers
            } else {
                playersList = []
            }
        }
        
        function onErrorOccurred(message) {
            console.log("Error loading replay:", message)
            isLoading = false
        }
    }
    
    Component.onCompleted: {
        if (sessionId > 0) {
            isLoading = true
            networkClient.sendGetReplayDetails(sessionId)
        }
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
                text: "🎬 Replay Game"
                font.family: "Lexend"
                font.pixelSize: 20
                font.bold: true
                color: "#FFFFFF"
            }
        }
        
        // Spacing
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 24
        }
        
        // Replay content
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            
            Column {
                width: parent.width
                spacing: 16
                
                // Game info card
                Rectangle {
                    width: parent.width
                    radius: 16
                    color: "#3D2B56"
                    border.color: "#5D4586"
                    border.width: 1
                    
                    Column {
                        width: parent.width
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.margins: 20
                        spacing: 12
                        
                        Text {
                            text: "Thông tin trận đấu"
                            font.family: "Lexend"
                            font.pixelSize: 18
                            font.bold: true
                            color: "#FFFFFF"
                        }
                        
                        Text {
                            text: "Session ID: " + sessionId
                            font.family: "Lexend"
                            font.pixelSize: 14
                            color: "#B0B0B0"
                        }
                        
                        Text {
                            text: {
                                if (replayData.started_at) {
                                    var date = new Date(replayData.started_at)
                                    return "Bắt đầu: " + date.toLocaleString("vi-VN")
                                }
                                return "Ngày chơi: N/A"
                            }
                            font.family: "Lexend"
                            font.pixelSize: 14
                            color: "#B0B0B0"
                        }
                        
                        Text {
                            text: {
                                var statusText = replayData.status || "N/A"
                                if (statusText === "FINISHED") return "Trạng thái: Hoàn thành"
                                if (statusText === "ABORTED") return "Trạng thái: Hủy bỏ"
                                return "Trạng thái: " + statusText
                            }
                            font.family: "Lexend"
                            font.pixelSize: 14
                            color: "#B0B0B0"
                        }
                    }
                    
                    implicitHeight: childrenRect.height + 40
                }
                
                // Loading indicator
                Rectangle {
                    width: parent.width
                    height: 200
                    radius: 16
                    color: "#3D2B56"
                    visible: isLoading
                    
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
                            text: "Đang tải replay..."
                            font.family: "Lexend"
                            font.pixelSize: 16
                            color: "#FFFFFF"
                        }
                    }
                }
                
                // Empty state
                Rectangle {
                    width: parent.width
                    height: 200
                    radius: 16
                    color: "#3D2B56"
                    border.color: "#5D4586"
                    border.width: 1
                    visible: !isLoading && roundsList.length === 0
                    
                    Column {
                        anchors.centerIn: parent
                        spacing: 16
                        
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "📭"
                            font.pixelSize: 48
                        }
                        
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "Không có dữ liệu replay"
                            font.family: "Lexend"
                            font.pixelSize: 16
                            font.bold: true
                            color: "#FFFFFF"
                        }
                    }
                }
                
                // Rounds list
                Repeater {
                    model: roundsList
                    
                    Rectangle {
                        width: parent.width
                        radius: 16
                        color: "#3D2B56"
                        border.color: "#5D4586"
                        border.width: 1
                        
                        Column {
                            width: parent.width
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.margins: 20
                            spacing: 16
                            
                            // Round header
                            RowLayout {
                                width: parent.width
                                
                                Text {
                                    text: "Round " + (modelData.round_number || index + 1)
                                    font.family: "Lexend"
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "#FFFFFF"
                                }
                                
                                Item {
                                    Layout.fillWidth: true
                                }
                                
                                Rectangle {
                                    Layout.preferredWidth: 60
                                    Layout.preferredHeight: 24
                                    radius: 12
                                    color: {
                                        var diff = modelData.difficulty || "EASY"
                                        if (diff === "EASY") return "#4CAF50"
                                        if (diff === "MEDIUM") return "#FF9800"
                                        return "#F44336"
                                    }
                                    
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.difficulty || "EASY"
                                        font.family: "Lexend"
                                        font.pixelSize: 11
                                        font.bold: true
                                        color: "#FFFFFF"
                                    }
                                }
                            }
                            
                            // Question content
                            Text {
                                width: parent.width
                                text: modelData.content || ""
                                font.family: "Lexend"
                                font.pixelSize: 14
                                color: "#FFFFFF"
                                wrapMode: Text.WordWrap
                            }
                            
                            // Options
                            Column {
                                width: parent.width
                                spacing: 8
                                
                                Repeater {
                                    model: [
                                        { label: "A", text: modelData.opA || "" },
                                        { label: "B", text: modelData.opB || "" },
                                        { label: "C", text: modelData.opC || "" },
                                        { label: "D", text: modelData.opD || "" }
                                    ]
                                    
                                    Rectangle {
                                        width: parent.width
                                        height: 32
                                        radius: 8
                                        color: {
                                            var correct = modelData.label === (modelData.correct_op || "")
                                            return correct ? "#4CAF50" : "#5D4586"
                                        }
                                        
                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.leftMargin: 12
                                            anchors.rightMargin: 12
                                            spacing: 12
                                            
                                            Text {
                                                text: modelData.label + "."
                                                font.family: "Lexend"
                                                font.pixelSize: 14
                                                font.bold: true
                                                color: "#FFFFFF"
                                            }
                                            
                                            Text {
                                                Layout.fillWidth: true
                                                text: modelData.text
                                                font.family: "Lexend"
                                                font.pixelSize: 14
                                                color: "#FFFFFF"
                                                elide: Text.ElideRight
                                            }
                                        }
                                    }
                                }
                            }
                            
                            // Answers section
                            Text {
                                text: "Câu trả lời của players:"
                                font.family: "Lexend"
                                font.pixelSize: 14
                                font.bold: true
                                color: "#B0B0B0"
                                visible: modelData.answers && modelData.answers.length > 0
                            }
                            
                            Column {
                                width: parent.width
                                spacing: 8
                                visible: modelData.answers && modelData.answers.length > 0
                                
                                Repeater {
                                    model: modelData.answers || []
                                    
                                    Rectangle {
                                        width: parent.width
                                        radius: 8
                                        color: "#2E1A47"
                                        
                                        RowLayout {
                                            width: parent.width
                                            anchors.left: parent.left
                                            anchors.top: parent.top
                                            anchors.margins: 12
                                            
                                            Text {
                                                text: "User " + (modelData.user_id || "?")
                                                font.family: "Lexend"
                                                font.pixelSize: 13
                                                color: "#FFFFFF"
                                            }
                                            
                                            Item {
                                                Layout.fillWidth: true
                                            }
                                            
                                            Text {
                                                text: {
                                                    var answer = modelData.answer
                                                    if (!answer || answer === "null" || answer === "") {
                                                        return "⏱ Timeout"
                                                    }
                                                    return "Đáp án: " + answer
                                                }
                                                font.family: "Lexend"
                                                font.pixelSize: 13
                                                color: {
                                                    if (modelData.is_correct) return "#4CAF50"
                                                    return "#F44336"
                                                }
                                                font.bold: true
                                            }
                                            
                                            Text {
                                                text: {
                                                    var score = modelData.score_gained || 0
                                                    return "+" + score
                                                }
                                                font.family: "Lexend"
                                                font.pixelSize: 13
                                                font.bold: true
                                                color: "#FFD700"
                                                visible: score > 0
                                            }
                                        }
                                        
                                        implicitHeight: childrenRect.height + 24
                                    }
                                }
                            }
                            
                            // Explanation
                            Text {
                                width: parent.width
                                text: "Giải thích: " + (modelData.explanation || "N/A")
                                font.family: "Lexend"
                                font.pixelSize: 12
                                color: "#B0B0B0"
                                wrapMode: Text.WordWrap
                                visible: modelData.explanation
                            }
                        }
                        
                        implicitHeight: childrenRect.height + 40
                    }
                }
            }
        }
    }
}
