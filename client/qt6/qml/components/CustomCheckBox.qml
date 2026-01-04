import QtQuick 6.0
import QtQuick.Controls 6.0

Rectangle {
    id: checkbox
    width: 20
    height: 20
    color: "#3D2B56"
    border.color: checked ? "#FFC107" : "#5D4586"
    border.width: 2
    radius: 4
    
    property bool checked: false
    
    // Checkmark
    Text {
        anchors.centerIn: parent
        text: "✓"
        color: "#FFC107"
        font.pixelSize: 14
        font.bold: true
        visible: checkbox.checked
    }
    
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: checkbox.checked = !checkbox.checked
    }
}

