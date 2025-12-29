import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

ApplicationWindow {
    id: root
    width: 400
    height: 700
    minimumWidth: 360
    minimumHeight: 640
    visible: true
    title: "Ai là triệu phú"
    
    // Style Guide: Background color
    color: "#2E1A47"
    
    StackView {
        id: stackView
        anchors.fill: parent
        
        Component.onCompleted: {
            push("SplashScreen.qml", {"stackView": stackView})
        }
        
        // Smooth transitions
        pushEnter: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 300
            }
        }
        popExit: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: 300
            }
        }
    }
}

