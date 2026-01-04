import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: winScreen
    width: parent ? parent.width : 500
    height: parent ? parent.height : 600
    
    property StackView stackView
    property string username: ""
    property int score: 0
    
    // Background gradient
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#11998e" }
            GradientStop { position: 1.0; color: "#38ef7d" }
        }
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 40
        spacing: 30
        
        Item { Layout.fillHeight: true }
        
        // Title
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            radius: 15
            color: "white"
            border.color: "#FFD700"
            border.width: 3
            
            Text {
                anchors.centerIn: parent
                text: "🎉 CHÚC MỪNG! 🎉"
                font.family: "Lexend"
                font.pixelSize: 36
                font.bold: true
                color: "#FFD700"
            }
        }
        
        // Message
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 200
            radius: 10
            color: Qt.rgba(1.0, 1.0, 1.0, 0.2)
            
            Text {
                anchors.centerIn: parent
                width: parent.width - 40
                text: "🏆 Bạn đã hoàn thành 15 câu hỏi! 🏆\n\n" +
                      "💰 Điểm số: " + score + "/15\n\n" +
                      "✨ Bạn là một triệu phú thực thụ! ✨"
                font.family: "Lexend"
                font.pixelSize: 16
                font.bold: true
                color: "#FFFFFF"
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
        }
        
        // Play Again Button
        Button {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            text: "🔄 Chơi lại"
            font.family: "Lexend"
            font.pixelSize: 14
            font.bold: true
            
            background: Rectangle {
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#4CAF50" }
                    GradientStop { position: 1.0; color: "#45a049" }
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
                    stackView.replace("QuickModeGame.qml", {
                        "stackView": stackView,
                        "username": username
                    })
                }
            }
        }
        
        // Back to Menu Button
        Button {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            text: "🏠 Về menu chính"
            font.family: "Lexend"
            font.pixelSize: 14
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
        
        Item { Layout.fillHeight: true }
    }
}

