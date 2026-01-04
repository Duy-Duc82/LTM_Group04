import QtQuick 6.0
import QtQuick.Controls 6.0

Rectangle {
    id: toast
    width: parent.width * 0.8
    height: 48
    radius: 8
    color: "#3D2B56"
    border.color: "#5D4586"
    border.width: 1
    anchors.horizontalCenter: parent.horizontalCenter
    y: -height
    opacity: 0
    z: 1000
    
    property string message: ""
    property string messageColor: "#FFFFFF"
    
    Text {
        anchors.centerIn: parent
        text: toast.message
        color: toast.messageColor
        font.family: "Lexend"
        font.pixelSize: 14
    }
    
    function show(msg, color) {
        toast.message = msg
        toast.messageColor = color || "#FFFFFF"
        
        showAnimation.start()
        hideTimer.start()
    }
    
    SequentialAnimation {
        id: showAnimation
        ParallelAnimation {
            NumberAnimation {
                target: toast
                property: "y"
                to: 20
                duration: 300
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                target: toast
                property: "opacity"
                to: 1
                duration: 300
            }
        }
    }
    
    Timer {
        id: hideTimer
        interval: 2000
        onTriggered: {
            hideAnimation.start()
        }
    }
    
    SequentialAnimation {
        id: hideAnimation
        ParallelAnimation {
            NumberAnimation {
                target: toast
                property: "y"
                to: -toast.height
                duration: 300
                easing.type: Easing.InCubic
            }
            NumberAnimation {
                target: toast
                property: "opacity"
                to: 0
                duration: 300
            }
        }
    }
}

