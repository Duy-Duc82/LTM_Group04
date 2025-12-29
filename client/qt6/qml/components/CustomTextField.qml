import QtQuick 6.0
import QtQuick.Controls 6.0

Column {
    id: textFieldColumn
    spacing: 4
    width: parent.width
    
    property alias text: textInput.text
    property alias placeholderText: textInput.placeholderText
    property alias echoMode: textInput.echoMode
    property alias validator: textInput.validator
    property alias inputMethodHints: textInput.inputMethodHints
    property bool hasError: false
    property string errorMessage: ""
    
    TextField {
        id: textInput
        width: parent.width
        height: 48
        font.family: "Lexend"
        font.pixelSize: 14
        placeholderTextColor: "#A0A0A0"
        
        // No validator - allow any input
        
        background: Rectangle {
            color: "#3D2B56"
            border.color: textFieldColumn.hasError ? "#FF5252" : (textInput.activeFocus ? "#5D4586" : "#3D2B56")
            border.width: textInput.activeFocus ? 2 : 1
            radius: 8
        }
        
        color: "#FFFFFF"
        padding: 12
        
        onTextChanged: {
            if (textFieldColumn.hasError && text.length > 0) {
                textFieldColumn.hasError = false
            }
        }
    }
    
    // Error message (hidden by default)
    Text {
        id: errorText
        text: textFieldColumn.errorMessage
        color: "#FF5252"
        font.family: "Lexend"
        font.pixelSize: 12
        visible: textFieldColumn.hasError && textFieldColumn.errorMessage.length > 0
        height: visible ? contentHeight : 0
        width: parent.width
    }
}

