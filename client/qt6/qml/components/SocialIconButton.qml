import QtQuick 6.0
import QtQuick.Controls 6.0

Rectangle {
    id: socialButton
    width: 45
    height: 45
    radius: width / 2
    color: "#3D2B56"
    border.color: "#5D4586"
    border.width: 1
    
    signal clicked()
    
    // Placeholder for icon - can be replaced with Image or Icon font
    Text {
        anchors.centerIn: parent
        text: "G" // Placeholder - replace with actual icon
        font.pixelSize: 20
        color: "#FFFFFF"
    }
    
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: socialButton.clicked()
    }
    
    property string iconSource: ""
}

