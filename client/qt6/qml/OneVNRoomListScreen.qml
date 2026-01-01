import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: roomListScreen
    width: parent ? parent.width : 360
    height: parent ? parent.height : 640
    
    property StackView stackView
    property string username: ""
    property var roomsList: []
    property bool isLoading: false
    property string joinRoomIdText: ""
    
    // Connect to NetworkClient
    Connections {
        target: networkClient
        
        function onRoomsListReceived(rooms) {
            console.log("=== Rooms list received ===")
            console.log("Rooms count:", rooms.length)
            console.log("Rooms data:", JSON.stringify(rooms))
            
            var tempList = []
            for (var i = 0; i < rooms.length; i++) {
                var room = rooms[i]
                tempList.push({
                    room_id: room.room_id || 0,
                    owner_id: room.owner_id || 0,
                    owner_username: room.owner_username || "",
                    member_count: room.member_count || 0,
                    max_players: room.max_players || 8,
                    created_at: room.created_at || ""
                })
            }
            roomsList = tempList
            isLoading = false
            console.log("Updated roomsList length:", roomsList.length)
        }
        
        function onOneVNRoomJoined(success, errorMsg) {
            if (success) {
                // Navigate to OneVNMode
                if (stackView) {
                    stackView.push("OneVNMode.qml", {
                        "stackView": stackView,
                        "username": username
                    })
                }
            } else {
                errorDialog.text = "Tham gia phòng thất bại: " + errorMsg
                errorDialog.open()
            }
        }
        
        function onErrorOccurred(message) {
            console.log("Error:", message)
            isLoading = false
            errorDialog.text = message
            errorDialog.open()
        }
    }
    
    Component.onCompleted: {
        isLoading = true
        networkClient.sendListRooms()
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
                text: "🏠 Danh sách phòng"
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
        
        // Join by ID section
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            radius: 16
            color: "#3D2B56"
            border.color: "#5D4586"
            border.width: 1
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12
                
                Text {
                    text: "Tham gia phòng theo ID"
                    font.family: "Lexend"
                    font.pixelSize: 14
                    font.bold: true
                    color: "#FFFFFF"
                }
                
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    
                    TextField {
                        id: roomIdInput
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        placeholderText: "Nhập Room ID"
                        font.family: "Lexend"
                        font.pixelSize: 14
                        color: "#FFFFFF"
                        
                        background: Rectangle {
                            color: "#2E1A47"
                            border.color: "#5D4586"
                            border.width: 1
                            radius: 8
                        }
                        
                        onTextChanged: {
                            joinRoomIdText = text
                        }
                    }
                    
                    Button {
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 40
                        text: "Tham gia"
                        font.family: "Lexend"
                        font.pixelSize: 14
                        font.bold: true
                        enabled: joinRoomIdText !== ""
                        
                        background: Rectangle {
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: enabled ? "#4CAF50" : "#666666" }
                                GradientStop { position: 1.0; color: enabled ? "#45a049" : "#555555" }
                            }
                            radius: 8
                        }
                        
                        contentItem: Text {
                            text: parent.text
                            font: parent.font
                            color: "#FFFFFF"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        
                        onClicked: {
                            var roomId = parseInt(joinRoomIdText)
                            if (roomId > 0) {
                                networkClient.sendJoinRoom(roomId)
                            } else {
                                errorDialog.text = "ID phòng không hợp lệ"
                                errorDialog.open()
                            }
                        }
                    }
                }
            }
        }
        
        // Spacing
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 16
        }
        
        // Rooms list title
        Text {
            Layout.fillWidth: true
            text: "Phòng đang chờ (" + roomsList.length + ")"
            font.family: "Lexend"
            font.pixelSize: 16
            font.bold: true
            color: "#FFFFFF"
        }
        
        // Spacing
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 12
        }
        
        // Rooms list
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            
            Column {
                width: parent.width
                spacing: 12
                
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
                            text: "Đang tải danh sách phòng..."
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
                    visible: !isLoading && roomsList.length === 0
                    
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
                            text: "Không có phòng nào"
                            font.family: "Lexend"
                            font.pixelSize: 16
                            font.bold: true
                            color: "#FFFFFF"
                        }
                        
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "Tạo phòng mới để bắt đầu"
                            font.family: "Lexend"
                            font.pixelSize: 14
                            color: "#B0B0B0"
                        }
                    }
                }
                
                // Room items
                Repeater {
                    model: roomsList
                    
                    Rectangle {
                        width: parent.width
                        height: 100
                        radius: 16
                        color: "#3D2B56"
                        border.color: "#5D4586"
                        border.width: 1
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 16
                            spacing: 16
                            
                            // Room info
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                
                                Text {
                                    text: "Chủ phòng: " + (modelData.owner_username || "N/A")
                                    font.family: "Lexend"
                                    font.pixelSize: 14
                                    font.bold: true
                                    color: "#FFFFFF"
                                    elide: Text.ElideRight
                                }
                                
                                Text {
                                    text: "Room ID: " + (modelData.room_id || 0)
                                    font.family: "Lexend"
                                    font.pixelSize: 12
                                    color: "#B0B0B0"
                                }
                                
                                Text {
                                    text: "Người chơi: " + (modelData.member_count || 0) + "/" + (modelData.max_players || 8)
                                    font.family: "Lexend"
                                    font.pixelSize: 12
                                    color: "#B0B0B0"
                                }
                            }
                            
                            // Join button
                            Button {
                                Layout.preferredWidth: 80
                                Layout.preferredHeight: 40
                                text: "Tham gia"
                                font.family: "Lexend"
                                font.pixelSize: 12
                                font.bold: true
                                
                                background: Rectangle {
                                    gradient: Gradient {
                                        GradientStop { position: 0.0; color: "#4CAF50" }
                                        GradientStop { position: 1.0; color: "#45a049" }
                                    }
                                    radius: 8
                                }
                                
                                contentItem: Text {
                                    text: parent.text
                                    font: parent.font
                                    color: "#FFFFFF"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                
                                onClicked: {
                                    var roomId = modelData.room_id || 0
                                    if (roomId > 0) {
                                        networkClient.sendJoinRoom(roomId)
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Error dialog
    Dialog {
        id: errorDialog
        title: "Lỗi"
        modal: true
        anchors.centerIn: parent
        width: 300
        height: 150
        
        property string text: ""
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 20
            
            Text {
                Layout.fillWidth: true
                text: errorDialog.text
                font.family: "Lexend"
                font.pixelSize: 14
                color: "#333333"
                wrapMode: Text.WordWrap
            }
            
            Button {
                Layout.fillWidth: true
                text: "Đóng"
                onClicked: errorDialog.close()
            }
        }
    }
}

