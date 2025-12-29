import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import QtQml 6.0
import "components"

Item {
    id: oneVNMode
    width: parent ? parent.width : 1000
    height: parent ? parent.height : 700
    
    property StackView stackView
    property string username: ""
    
    // Game state
    property int roomId: 0
    property real sessionId: 0  // Use real to handle large session IDs
    property bool isOwner: false
    property int currentRound: 0
    property int totalRounds: 0
    property int myScore: 0
    property bool eliminated: false
    property bool waitingForAnswer: false
    property int timeRemaining: 15
    property string selectedAnswer: ""
    
    // Queued question (to show after score message closes)
    property var queuedQuestion: null
    property bool showingScoreMessage: false
    property bool waitingForNextQuestion: false
    
    // Question data
    property string questionContent: ""
    property string optionA: ""
    property string optionB: ""
    property string optionC: ""
    property string optionD: ""
    property string difficulty: ""
    
    // Members list
    property var membersList: []
    // Map user_id to username (for leaderboard that only has user_id)
    property var userIdToUsername: ({})
    
    // Background gradient
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#667eea" }
            GradientStop { position: 1.0; color: "#764ba2" }
        }
    }
    
    // Internal StackView for screens
    StackView {
        id: screenStack
        anchors.fill: parent
        initialItem: roomSelectionScreen
    }
    
    // Room Selection Screen
    Component {
        id: roomSelectionScreen
        
        ScrollView {
            clip: true
            
            Item {
                width: parent.width
                height: childrenRect.height
                
                ColumnLayout {
                    width: parent.width - 40
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: 20
                    spacing: 20
                
                // Back button and title row
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12
                    
                    // Back button
                    Rectangle {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        radius: 20
                        color: Qt.rgba(1.0, 1.0, 1.0, 0.2)
                        
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
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        
                        Text {
                            Layout.fillWidth: true
                            text: "🏆 1vN MODE 🏆"
                            font.family: "Lexend"
                            font.pixelSize: 24
                            font.bold: true
                            color: "#FFFFFF"
                            horizontalAlignment: Text.AlignHCenter
                        }
                        
                        Text {
                            Layout.fillWidth: true
                            text: "Chế độ đối kháng"
                            font.family: "Lexend"
                            font.pixelSize: 14
                            color: Qt.rgba(1.0, 1.0, 1.0, 0.9)
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                    
                    // Spacer to balance layout
                    Item {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                    }
                }
                
                // Create Room Section
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 320
                    radius: 20
                    color: "#FFFFFF"
                    border.color: "#E0E0E0"
                    border.width: 1
                    
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 24
                        spacing: 20
                        
                        // Section title
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            
                            Text {
                                text: "➕"
                                font.pixelSize: 20
                            }
                            
                            Text {
                                Layout.fillWidth: true
                                text: "Tạo phòng mới"
                                font.family: "Lexend"
                                font.pixelSize: 20
                                font.bold: true
                                color: "#667eea"
                            }
                        }
                        
                        // Description
                        Text {
                            Layout.fillWidth: true
                            text: "Thiết lập số lượng câu hỏi cho từng mức độ"
                            font.family: "Lexend"
                            font.pixelSize: 12
                            color: "#666666"
                            wrapMode: Text.WordWrap
                        }
                        
                        // Difficulty settings
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 16
                            
                            // Easy questions
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 80
                                    radius: 10
                                    color: "#E8F5E9"
                                    border.color: "#4CAF50"
                                    border.width: 2
                                    
                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        spacing: 4
                                        
                                        Text {
                                            Layout.fillWidth: true
                                            text: "Câu dễ"
                                            font.family: "Lexend"
                                            font.pixelSize: 12
                                            font.bold: true
                                            color: "#2E7D32"
                                            horizontalAlignment: Text.AlignHCenter
                                        }
                                        
                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 8
                                            
                                            Rectangle {
                                                Layout.preferredWidth: 32
                                                Layout.preferredHeight: 32
                                                radius: 16
                                                color: "#4CAF50"
                                                
                                                Text {
                                                    anchors.centerIn: parent
                                                    text: "-"
                                                    font.pixelSize: 18
                                                    font.bold: true
                                                    color: "#FFFFFF"
                                                }
                                                
                                                MouseArea {
                                                    anchors.fill: parent
                                                    onClicked: {
                                                        if (easyCountSpin.value > easyCountSpin.from) {
                                                            easyCountSpin.value--
                                                        }
                                                    }
                                                }
                                            }
                                            
                                            Text {
                                                Layout.fillWidth: true
                                                text: easyCountSpin.value.toString()
                                                font.family: "Lexend"
                                                font.pixelSize: 18
                                                font.bold: true
                                                color: "#2E7D32"
                                                horizontalAlignment: Text.AlignHCenter
                                            }
                                            
                                            Rectangle {
                                                Layout.preferredWidth: 32
                                                Layout.preferredHeight: 32
                                                radius: 16
                                                color: "#4CAF50"
                                                
                                                Text {
                                                    anchors.centerIn: parent
                                                    text: "+"
                                                    font.pixelSize: 18
                                                    font.bold: true
                                                    color: "#FFFFFF"
                                                }
                                                
                                                MouseArea {
                                                    anchors.fill: parent
                                                    onClicked: {
                                                        if (easyCountSpin.value < easyCountSpin.to) {
                                                            easyCountSpin.value++
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                
                                SpinBox {
                                    id: easyCountSpin
                                    Layout.fillWidth: true
                                    from: 0
                                    to: 10
                                    value: 5
                                    visible: false
                                }
                            }
                            
                            // Medium questions
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 80
                                    radius: 10
                                    color: "#FFF3E0"
                                    border.color: "#FF9800"
                                    border.width: 2
                                    
                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        spacing: 4
                                        
                                        Text {
                                            Layout.fillWidth: true
                                            text: "Câu trung bình"
                                            font.family: "Lexend"
                                            font.pixelSize: 12
                                            font.bold: true
                                            color: "#E65100"
                                            horizontalAlignment: Text.AlignHCenter
                                        }
                                        
                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 8
                                            
                                            Rectangle {
                                                Layout.preferredWidth: 32
                                                Layout.preferredHeight: 32
                                                radius: 16
                                                color: "#FF9800"
                                                
                                                Text {
                                                    anchors.centerIn: parent
                                                    text: "-"
                                                    font.pixelSize: 18
                                                    font.bold: true
                                                    color: "#FFFFFF"
                                                }
                                                
                                                MouseArea {
                                                    anchors.fill: parent
                                                    onClicked: {
                                                        if (mediumCountSpin.value > mediumCountSpin.from) {
                                                            mediumCountSpin.value--
                                                        }
                                                    }
                                                }
                                            }
                                            
                                            Text {
                                                Layout.fillWidth: true
                                                text: mediumCountSpin.value.toString()
                                                font.family: "Lexend"
                                                font.pixelSize: 18
                                                font.bold: true
                                                color: "#E65100"
                                                horizontalAlignment: Text.AlignHCenter
                                            }
                                            
                                            Rectangle {
                                                Layout.preferredWidth: 32
                                                Layout.preferredHeight: 32
                                                radius: 16
                                                color: "#FF9800"
                                                
                                                Text {
                                                    anchors.centerIn: parent
                                                    text: "+"
                                                    font.pixelSize: 18
                                                    font.bold: true
                                                    color: "#FFFFFF"
                                                }
                                                
                                                MouseArea {
                                                    anchors.fill: parent
                                                    onClicked: {
                                                        if (mediumCountSpin.value < mediumCountSpin.to) {
                                                            mediumCountSpin.value++
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                
                                SpinBox {
                                    id: mediumCountSpin
                                    Layout.fillWidth: true
                                    from: 0
                                    to: 10
                                    value: 5
                                    visible: false
                                }
                            }
                            
                            // Hard questions
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 80
                                    radius: 10
                                    color: "#FFEBEE"
                                    border.color: "#F44336"
                                    border.width: 2
                                    
                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        spacing: 4
                                        
                                        Text {
                                            Layout.fillWidth: true
                                            text: "Câu khó"
                                            font.family: "Lexend"
                                            font.pixelSize: 12
                                            font.bold: true
                                            color: "#C62828"
                                            horizontalAlignment: Text.AlignHCenter
                                        }
                                        
                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 8
                                            
                                            Rectangle {
                                                Layout.preferredWidth: 32
                                                Layout.preferredHeight: 32
                                                radius: 16
                                                color: "#F44336"
                                                
                                                Text {
                                                    anchors.centerIn: parent
                                                    text: "-"
                                                    font.pixelSize: 18
                                                    font.bold: true
                                                    color: "#FFFFFF"
                                                }
                                                
                                                MouseArea {
                                                    anchors.fill: parent
                                                    onClicked: {
                                                        if (hardCountSpin.value > hardCountSpin.from) {
                                                            hardCountSpin.value--
                                                        }
                                                    }
                                                }
                                            }
                                            
                                            Text {
                                                Layout.fillWidth: true
                                                text: hardCountSpin.value.toString()
                                                font.family: "Lexend"
                                                font.pixelSize: 18
                                                font.bold: true
                                                color: "#C62828"
                                                horizontalAlignment: Text.AlignHCenter
                                            }
                                            
                                            Rectangle {
                                                Layout.preferredWidth: 32
                                                Layout.preferredHeight: 32
                                                radius: 16
                                                color: "#F44336"
                                                
                                                Text {
                                                    anchors.centerIn: parent
                                                    text: "+"
                                                    font.pixelSize: 18
                                                    font.bold: true
                                                    color: "#FFFFFF"
                                                }
                                                
                                                MouseArea {
                                                    anchors.fill: parent
                                                    onClicked: {
                                                        if (hardCountSpin.value < hardCountSpin.to) {
                                                            hardCountSpin.value++
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                
                                SpinBox {
                                    id: hardCountSpin
                                    Layout.fillWidth: true
                                    from: 0
                                    to: 10
                                    value: 5
                                    visible: false
                                }
                            }
                        }
                        
                        // Create button
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 50
                            radius: 12
                            color: "#4CAF50"
                            
                            Text {
                                anchors.centerIn: parent
                                text: "Tạo phòng"
                                font.family: "Lexend"
                                font.pixelSize: 16
                                font.bold: true
                                color: "#FFFFFF"
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    networkClient.sendCreateRoom(easyCountSpin.value, mediumCountSpin.value, hardCountSpin.value)
                                }
                            }
                        }
                    }
                }
                
                // Join Room Section
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 180
                    radius: 20
                    color: "#FFFFFF"
                    border.color: "#E0E0E0"
                    border.width: 1
                    
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 24
                        spacing: 20
                        
                        // Section title
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            
                            Text {
                                text: "🔗"
                                font.pixelSize: 20
                            }
                            
                            Text {
                                Layout.fillWidth: true
                                text: "Tham gia phòng"
                                font.family: "Lexend"
                                font.pixelSize: 20
                                font.bold: true
                                color: "#667eea"
                            }
                        }
                        
                        // Room ID input
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            
                            Text {
                                text: "Room ID:"
                                font.family: "Lexend"
                                font.pixelSize: 14
                                color: "#333333"
                            }
                            
                            CustomTextField {
                                id: roomIdInput
                                Layout.fillWidth: true
                                Layout.preferredHeight: 45
                                placeholderText: "Nhập Room ID"
                            }
                        }
                        
                        // Join button
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 50
                            radius: 12
                            color: "#2196F3"
                            
                            Text {
                                anchors.centerIn: parent
                                text: "Tham gia"
                                font.family: "Lexend"
                                font.pixelSize: 16
                                font.bold: true
                                color: "#FFFFFF"
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    var roomId = parseInt(roomIdInput.text)
                                    if (roomId > 0) {
                                        networkClient.sendJoinRoom(roomId)
                                    } else {
                                        // Show error
                                        console.log("Invalid Room ID")
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
    
    // Waiting Room Screen
    Component {
        id: waitingRoomScreen
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 40
            spacing: 20
            
            Text {
                Layout.fillWidth: true
                text: "Đang chờ trong phòng..."
                font.family: "Lexend"
                font.pixelSize: 24
                font.bold: true
                color: "#FFFFFF"
                horizontalAlignment: Text.AlignHCenter
            }
            
            Text {
                Layout.fillWidth: true
                text: "Room ID: " + roomId
                font.family: "Lexend"
                font.pixelSize: 16
                color: Qt.rgba(1.0, 1.0, 1.0, 0.9)
                horizontalAlignment: Text.AlignHCenter
            }
            
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 15
                color: Qt.rgba(1.0, 1.0, 1.0, 0.95)
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 10
                    
                    Text {
                        text: "Thành viên trong phòng:"
                        font.family: "Lexend"
                        font.pixelSize: 16
                        font.bold: true
                        color: "#333333"
                    }
                    
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: membersList.length
                        delegate: Rectangle {
                            width: parent.width
                            height: 40
                            color: index % 2 === 0 ? "#F5F5F5" : "#FFFFFF"
                            
                            property var member: membersList[index] || {}
                            
                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                text: member.username || "Người chơi " + (index + 1)
                                font.family: "Lexend"
                                font.pixelSize: 14
                                color: "#333333"
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }
            
            RowLayout {
                Layout.fillWidth: true
                spacing: 15
                
                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    text: isOwner ? "Bắt đầu game" : "Đang chờ chủ phòng..."
                    enabled: isOwner
                    opacity: enabled ? 1.0 : 0.5
                    font.family: "Lexend"
                    font.pixelSize: 14
                    font.bold: true
                    
                    background: Rectangle {
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#4CAF50" }
                            GradientStop { position: 1.0; color: "#45a049" }
                        }
                        radius: 10
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: "#FFFFFF"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: {
                        if (roomId > 0) {
                            networkClient.sendStartGame1VN(roomId)
                        }
                    }
                }
                
                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    text: "Rời phòng"
                    font.family: "Lexend"
                    font.pixelSize: 14
                    font.bold: true
                    
                    background: Rectangle {
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#f44336" }
                            GradientStop { position: 1.0; color: "#d32f2f" }
                        }
                        radius: 10
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: "#FFFFFF"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: {
                        if (roomId > 0) {
                            networkClient.sendLeaveRoom(roomId)
                            screenStack.replace(roomSelectionScreen)
                            roomId = 0
                        }
                    }
                }
            }
        }
    }
    
    // Game Playing Screen
    Component {
        id: gamePlayingScreen
        
        RowLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 20
            
            // Left: Question and Options
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 15
                
                // Round and Timer
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 20
                    
                    Text {
                        text: "Câu " + currentRound + "/" + totalRounds
                        font.family: "Lexend"
                        font.pixelSize: 18
                        font.bold: true
                        color: "#FFFFFF"
                    }
                    
                    Text {
                        text: "⏱ " + timeRemaining + "s"
                        font.family: "Lexend"
                        font.pixelSize: 18
                        font.bold: true
                        color: timeRemaining <= 5 ? "#f44336" : "#FFFFFF"
                    }
                }
                
                // Question
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 150
                    radius: 15
                    color: "white"
                    
                    // Loading indicator (shown when waiting for next question)
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 15
                        visible: waitingForNextQuestion
                        
                        // Simple rotating loading indicator
                        Item {
                            Layout.alignment: Qt.AlignHCenter
                            width: 50
                            height: 50
                            
                            Rectangle {
                                id: spinnerCircle
                                anchors.centerIn: parent
                                width: 40
                                height: 40
                                radius: 20
                                color: "transparent"
                                border.color: "#667eea"
                                border.width: 3
                                
                                Rectangle {
                                    anchors.top: parent.top
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    width: 8
                                    height: 8
                                    radius: 4
                                    color: "#667eea"
                                }
                            }
                            
                            RotationAnimation {
                                target: spinnerCircle
                                running: waitingForNextQuestion
                                from: 0
                                to: 360
                                duration: 1000
                                loops: Animation.Infinite
                            }
                        }
                        
                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "Đang chờ câu hỏi tiếp theo..."
                            font.family: "Lexend"
                            font.pixelSize: 16
                            color: "#667eea"
                        }
                    }
                    
                    // Question content (hidden when waiting for next question)
                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: 20
                        clip: true
                        visible: !waitingForNextQuestion
                        
                        Text {
                            width: parent.width
                            text: questionContent
                            font.family: "Lexend"
                            font.pixelSize: 18
                            font.bold: true
                            color: "#333333"
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }
                
                // Options
                GridLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    columns: 2
                    columnSpacing: 15
                    rowSpacing: 15
                    
                    GameOptionButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 80
                        optionText: waitingForNextQuestion ? "" : ("A: " + optionA)
                        optionLetter: "A"
                        enabled: !waitingForAnswer && !eliminated && !waitingForNextQuestion && optionA.length > 0
                        forceWhiteWhenDisabled: waitingForNextQuestion
                        onClicked: handleGameAnswer("A")
                    }
                    
                    GameOptionButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 80
                        optionText: waitingForNextQuestion ? "" : ("B: " + optionB)
                        optionLetter: "B"
                        enabled: !waitingForAnswer && !eliminated && !waitingForNextQuestion && optionB.length > 0
                        forceWhiteWhenDisabled: waitingForNextQuestion
                        onClicked: handleGameAnswer("B")
                    }
                    
                    GameOptionButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 80
                        optionText: waitingForNextQuestion ? "" : ("C: " + optionC)
                        optionLetter: "C"
                        enabled: !waitingForAnswer && !eliminated && !waitingForNextQuestion && optionC.length > 0
                        forceWhiteWhenDisabled: waitingForNextQuestion
                        onClicked: handleGameAnswer("C")
                    }
                    
                    GameOptionButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 80
                        optionText: waitingForNextQuestion ? "" : ("D: " + optionD)
                        optionLetter: "D"
                        enabled: !waitingForAnswer && !eliminated && !waitingForNextQuestion && optionD.length > 0
                        forceWhiteWhenDisabled: waitingForNextQuestion
                        onClicked: handleGameAnswer("D")
                    }
                }
            }
            
            // Right: Leaderboard
            Rectangle {
                Layout.preferredWidth: 250
                Layout.fillHeight: true
                radius: 15
                color: Qt.rgba(1.0, 1.0, 1.0, 0.95)
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10
                    
                    Text {
                        text: "Bảng xếp hạng"
                        font.family: "Lexend"
                        font.pixelSize: 16
                        font.bold: true
                        color: "#333333"
                    }
                    
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        id: leaderboardList
                        model: membersList.length
                        delegate: Rectangle {
                            width: parent.width
                            height: 50
                            
                            property var member: membersList[index] || {}
                            property bool isEliminated: member.eliminated || false
                            property bool isMe: member.userId === networkClient.getUserId()
                            
                            color: isEliminated ? "#FFCDD2" : (isMe ? "#C8E6C9" : "#F5F5F5")
                            
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                
                                Text {
                                    text: "#" + (index + 1)
                                    font.family: "Lexend"
                                    font.pixelSize: 14
                                    font.bold: true
                                    color: "#333333"
                                }
                                
                                Text {
                                    Layout.fillWidth: true
                                    text: member.username || "Người chơi " + (index + 1)
                                    font.family: "Lexend"
                                    font.pixelSize: 14
                                    color: "#333333"
                                    elide: Text.ElideRight
                                }
                                
                                Text {
                                    text: (member.score || 0) + " điểm"
                                    font.family: "Lexend"
                                    font.pixelSize: 14
                                    font.bold: true
                                    color: "#4CAF50"
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Game Over Screen
    Component {
        id: gameOverScreen
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 40
            spacing: 20
            
            Text {
                Layout.fillWidth: true
                text: "🏆 KẾT THÚC GAME 🏆"
                font.family: "Lexend"
                font.pixelSize: 28
                font.bold: true
                color: "#FFFFFF"
                horizontalAlignment: Text.AlignHCenter
            }
            
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 15
                color: Qt.rgba(1.0, 1.0, 1.0, 0.95)
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 10
                    
                    Text {
                        text: "Bảng xếp hạng cuối cùng:"
                        font.family: "Lexend"
                        font.pixelSize: 18
                        font.bold: true
                        color: "#333333"
                    }
                    
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        id: finalLeaderboardList
                        model: membersList.length
                        delegate: Rectangle {
                            width: parent.width
                            height: 60
                            
                            property var member: membersList[index] || {}
                            
                            color: index === 0 ? "#FFD700" : (index % 2 === 0 ? "#F5F5F5" : "#FFFFFF")
                            
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 15
                                
                                Text {
                                    text: index === 0 ? "🥇" : (index === 1 ? "🥈" : (index === 2 ? "🥉" : "#" + (index + 1)))
                                    font.pixelSize: 20
                                }
                                
                                Text {
                                    Layout.fillWidth: true
                                    text: member.username || "Người chơi " + (index + 1)
                                    font.family: "Lexend"
                                    font.pixelSize: 16
                                    font.bold: index === 0
                                    color: "#333333"
                                    elide: Text.ElideRight
                                }
                                
                                Text {
                                    text: (member.score || 0) + " điểm"
                                    font.family: "Lexend"
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "#4CAF50"
                                }
                            }
                        }
                    }
                }
            }
            
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                text: "🏠 Về menu chính"
                font.family: "Lexend"
                font.pixelSize: 16
                font.bold: true
                
                background: Rectangle {
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#667eea" }
                        GradientStop { position: 1.0; color: "#764ba2" }
                    }
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
                        stackView.replace("HomeScreen.qml", {
                            "stackView": stackView,
                            "username": username
                        })
                    }
                }
            }
        }
    }
    
    // Toast Message
    ToastMessage {
        id: toastMessage
        anchors.fill: parent
    }
    
    // Timer for question countdown
    Timer {
        id: questionTimer
        interval: 1000
        running: false
        repeat: true
        onTriggered: {
            if (timeRemaining > 0) {
                timeRemaining--
            } else {
                // Timeout - submit empty answer
                questionTimer.stop()
                if (sessionId > 0 && currentRound > 0 && !waitingForAnswer) {
                    networkClient.sendSubmitAnswer1VN(sessionId, currentRound, "", 0)
                }
            }
        }
    }
    
    // Timer to close score message after 2 seconds and show queued question if any
    Timer {
        id: scoreMessageTimer
        interval: 2000  // 2 seconds
        running: false
        repeat: false
        onTriggered: {
            console.log("=== scoreMessageTimer triggered ===")
            console.log("Closing score message, queuedQuestion exists:", queuedQuestion !== null)
            
            // Close score message
            showingScoreMessage = false
            
            // Show queued question if any (new question arrived while message was showing)
            if (queuedQuestion) {
                console.log("Showing queued question after score message closed")
                console.log("Queued question round:", queuedQuestion.round, "content:", queuedQuestion.content)
                var q = queuedQuestion  // Save reference before clearing
                queuedQuestion = null  // Clear first to avoid issues
                waitingForNextQuestion = false
                
                // Show the queued question
                showQuestion(
                    q.round,
                    q.rounds,
                    q.diff,
                    q.questionId,
                    q.content,
                    q.options,
                    q.timeLimit
                )
            } else {
                console.log("No queued question, showing loading indicator")
                // Show loading indicator while waiting for next question
                waitingForNextQuestion = true
            }
        }
    }
    
    // Connect to NetworkClient signals
    Connections {
        target: networkClient
        
        function onOneVNRoomCreated(newRoomId) {
            roomId = newRoomId
            isOwner = true
            screenStack.replace(waitingRoomScreen)
            toastMessage.show("Đã tạo phòng: " + roomId, "#4CAF50")
        }
        
        function onOneVNRoomJoined(success, error) {
            if (success) {
                screenStack.replace(waitingRoomScreen)
                toastMessage.show("Đã tham gia phòng", "#4CAF50")
            } else {
                toastMessage.show("Không thể tham gia phòng: " + error, "#FF5252")
            }
        }
        
        function onOneVNRoomUpdate(members) {
            console.log("=== onOneVNRoomUpdate ===")
            console.log("Members array length:", members.length)
            var tempList = []
            var tempMap = {}
            
            for (var i = 0; i < members.length; i++) {
                var member = members[i]
                // Handle both QJsonObject (from C++) and plain JS object
                var userId = member.user_id || member.userId || 0
                var username = member.username || member.nickname || ""
                var score = member.score || 0
                var eliminated = member.eliminated || false
                
                console.log("Member", i, ":", userId, username, score, eliminated)
                
                // Store username mapping if available
                if (username && userId) {
                    tempMap[userId] = username
                }
                
                tempList.push({
                    userId: userId,
                    username: username || userIdToUsername[userId] || "",
                    score: score,
                    eliminated: eliminated
                })
            }
            
            // Update username map
            for (var key in tempMap) {
                userIdToUsername[key] = tempMap[key]
            }
            
            // Sort by score (descending), then by eliminated status (non-eliminated first)
            tempList.sort(function(a, b) {
                if (a.eliminated !== b.eliminated) {
                    return a.eliminated ? 1 : -1
                }
                return b.score - a.score
            })
            membersList = tempList
            console.log("Updated membersList length:", membersList.length)
        }
        
        function onOneVNGameStart1VN(gameSessionId, gameRoomId, rounds) {
            sessionId = gameSessionId
            roomId = gameRoomId
            totalRounds = rounds
            currentRound = 0
            myScore = 0
            eliminated = false
            waitingForAnswer = false
            showingScoreMessage = false
            queuedQuestion = null
            waitingForNextQuestion = false
            screenStack.replace(gamePlayingScreen)
        }
        
        function onOneVNQuestion1VNReceived(round, rounds, diff, questionId, content, options, timeLimit) {
            console.log("=== onOneVNQuestion1VNReceived ===")
            console.log("Round:", round, "Content:", content)
            console.log("showingScoreMessage:", showingScoreMessage, "queuedQuestion:", queuedQuestion !== null)
            
            // Clear loading indicator
            waitingForNextQuestion = false
            
            // Always queue the question if score message is showing
            // This ensures we wait for the score message to close before showing the new question
            if (showingScoreMessage) {
                console.log("Score message is showing, queueing question")
                queuedQuestion = {
                    round: round,
                    rounds: rounds,
                    diff: diff,
                    questionId: questionId,
                    content: content,
                    options: options,
                    timeLimit: timeLimit
                }
                console.log("Question queued, will show after score message closes (2 seconds)")
                // Don't start timer here - scoreMessageTimer is already running from onOneVNAnswerResult1VN
                // It will show the queued question when it triggers
            } else {
                // If score message is not showing, check if we just answered (timer might have already triggered)
                // In this case, show the question immediately (e.g., first question of the game)
                console.log("No score message showing, displaying question immediately")
                showQuestion(round, rounds, diff, questionId, content, options, timeLimit)
            }
        }
        
        function onOneVNAnswerResult1VN(correct, score, totalScore, isEliminated, timeout) {
            console.log("=== onOneVNAnswerResult1VN ===")
            console.log("Correct:", correct, "Score:", score, "TotalScore:", totalScore)
            
            questionTimer.stop()
            waitingForAnswer = false
            myScore = totalScore
            eliminated = isEliminated
            
            // Update my score in membersList
            var myUserId = networkClient.getUserId()
            for (var i = 0; i < membersList.length; i++) {
                if (membersList[i].userId === myUserId) {
                    membersList[i].score = totalScore
                    membersList[i].eliminated = isEliminated
                    break
                }
            }
            // Force UI update
            membersList = membersList
            
            // Mark that score message is showing
            showingScoreMessage = true
            console.log("showingScoreMessage set to true")
            
            // Show score message - it will stay visible for 2 seconds
            if (timeout) {
                toastMessage.show("Hết thời gian!", "#FF5252")
            } else if (correct) {
                toastMessage.show("Đúng! +" + score + " điểm", "#4CAF50")
            } else {
                toastMessage.show("Trả lời sai! Không được điểm", "#FF5252")
            }
            
            // Start timer to close score message after 2 seconds
            // This ensures the message is visible for 2 seconds before new question appears
            scoreMessageTimer.stop()
            scoreMessageTimer.start()
            console.log("Started scoreMessageTimer to close message after 2 seconds")
        }
        
        function onOneVNElimination(userId, round) {
            // Update members list to mark eliminated
            for (var i = 0; i < membersList.length; i++) {
                if (membersList[i].userId === userId) {
                    membersList[i].eliminated = true
                    break
                }
            }
            // Force UI update
            membersList = membersList
        }
        
        function onOneVNGameOver1VN(winnerId, leaderboard) {
            console.log("=== onOneVNGameOver1VN ===")
            console.log("Winner ID:", winnerId)
            console.log("Leaderboard length:", leaderboard.length)
            questionTimer.stop()
            membersList = []
            for (var i = 0; i < leaderboard.length; i++) {
                var player = leaderboard[i]
                // Handle both QJsonObject (from C++) and plain JS object
                var userId = player.user_id || player.userId || 0
                // Leaderboard from server only has user_id, not username
                // Use stored username from userIdToUsername map
                var username = player.username || player.nickname || userIdToUsername[userId] || ""
                var score = player.score || 0
                
                console.log("Player", i, ":", userId, "username:", username, "score:", score)
                console.log("userIdToUsername[" + userId + "] =", userIdToUsername[userId])
                
                membersList.push({
                    userId: userId,
                    username: username,
                    score: score,
                    eliminated: false
                })
            }
            console.log("Final membersList length:", membersList.length)
            // Force UI update
            membersList = membersList
            screenStack.replace(gameOverScreen)
        }
        
        function onErrorOccurred(error) {
            toastMessage.show("Lỗi: " + error, "#FF5252")
        }
    }
    
    // Function to show question - must be outside Connections so timer can call it
    function showQuestion(round, rounds, diff, questionId, content, options, timeLimit) {
        console.log("=== showQuestion ===")
        console.log("Round:", round, "Content:", content)
        console.log("Options object:", JSON.stringify(options))
        
        // Reset score message flag and loading indicator
        showingScoreMessage = false
        queuedQuestion = null
        waitingForNextQuestion = false
        
        // Update properties - force update by assigning values
        currentRound = round
        totalRounds = rounds
        difficulty = diff
        
        // Extract options - handle both QJsonObject and plain JS object
        var optA = ""
        var optB = ""
        var optC = ""
        var optD = ""
        
        if (options) {
            // Try to get options from object
            optA = options.A || options["A"] || ""
            optB = options.B || options["B"] || ""
            optC = options.C || options["C"] || ""
            optD = options.D || options["D"] || ""
        }
        
        console.log("Extracted options - A:", optA, "B:", optB, "C:", optC, "D:", optD)
        
        // Update properties
        questionContent = content || ""
        optionA = optA
        optionB = optB
        optionC = optC
        optionD = optD
        timeRemaining = timeLimit || 15
        waitingForAnswer = false
        selectedAnswer = ""
        
        // Force UI update by reassigning (triggers property change signals)
        questionContent = questionContent
        optionA = optionA
        optionB = optionB
        optionC = optionC
        optionD = optionD
        
        // Stop any existing timers
        scoreMessageTimer.stop()
        
        // Start timer
        questionTimer.stop()
        questionTimer.start()
        
        console.log("Question displayed: Round", currentRound, "Content:", questionContent)
        console.log("Options - A:", optionA, "B:", optionB, "C:", optionC, "D:", optionD)
    }
    
    function handleGameAnswer(answer) {
        if (waitingForAnswer || eliminated || sessionId === 0) return
        
        selectedAnswer = answer
        waitingForAnswer = true
        questionTimer.stop()
        
        var timeLeft = timeRemaining
        networkClient.sendSubmitAnswer1VN(sessionId, currentRound, answer, timeLeft)
    }
}

