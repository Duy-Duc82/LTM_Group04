import QtQuick 6.0
import QtQuick.Controls 6.0

Item {
    id: splashScreen
    width: parent ? parent.width : 400
    height: parent ? parent.height : 700
    
    property StackView stackView
    
    // Logo centered
    Rectangle {
        id: logo
        width: parent.width * 0.3
        height: parent.height * 0.25
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 64
        color: "transparent"
        
        // Placeholder for logo - replace with Image component
        Text {
            anchors.centerIn: parent
            text: "💰"
            font.pixelSize: 80
        }
    }
    
    // Game Title
    Text {
        id: gameTitle
        text: "AI LÀ TRIỆU PHÚ"
        font.family: "Lexend"
        font.pixelSize: 28
        font.bold: true
        color: "#FFFFFF"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: logo.bottom
        anchors.topMargin: 24
    }
    
    // Loading Bar Container
    Rectangle {
        id: loadingBarContainer
        width: 240
        height: 4
        radius: 2
        color: "#452763" // Dark purple background
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parent.height * 0.15 // 15% from bottom
        
        // Loading Bar Foreground
        Rectangle {
            id: loadingBar
            width: 0
            height: parent.height
            radius: parent.radius
            color: "#FFC107" // Amber/Gold
            
            // Animation
            NumberAnimation {
                id: loadingAnimation
                target: loadingBar
                property: "width"
                from: 0
                to: 240
                duration: 2800
                running: true
                
                onFinished: {
                    // Navigate to Signin after 200ms delay
                    timer.start()
                }
            }
        }
    }
    
    // Timer for final delay
    Timer {
        id: timer
        interval: 200
        onTriggered: {
            if (stackView) {
                stackView.replace("Signin.qml", {"stackView": stackView})
            }
        }
    }
}

