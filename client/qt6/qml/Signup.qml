import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "components"

ScrollView {
    id: signupScroll
    width: parent ? parent.width : 400
    height: parent ? parent.height : 700
    clip: true
    
    property StackView stackView
    
    ColumnLayout {
        width: parent.width - 48
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 0
        
        // Top Bar
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 20
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            Layout.bottomMargin: 24
            
            // Back Button
            Button {
                id: backButton
                text: "<"
                font.pixelSize: 24
                background: Rectangle {
                    color: "transparent"
                }
                contentItem: Text {
                    text: backButton.text
                    color: "#FFC107"
                    font: backButton.font
                }
                onClicked: {
                    if (stackView) {
                        stackView.pop()
                    }
                }
            }
            
            // Title
            Text {
                Layout.fillWidth: true
                text: "TẠO TÀI KHOẢN"
                font.family: "Lexend"
                font.pixelSize: 28
                font.bold: true
                color: "#FFFFFF"
                horizontalAlignment: Text.AlignHCenter
            }
            
            // Spacer for centering
            Item {
                width: backButton.width
            }
        }
        
        // Form Fields
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 24
            Layout.rightMargin: 24
            Layout.topMargin: 40
            spacing: 16
            
            // Username
            CustomTextField {
                id: usernameField
                Layout.fillWidth: true
                placeholderText: "Tên tài khoản"
            }
            
            // Password
            CustomTextField {
                id: passwordField
                Layout.fillWidth: true
                placeholderText: "Mật khẩu"
                echoMode: TextInput.Password
            }
            
            // Confirm Password
            CustomTextField {
                id: confirmPasswordField
                Layout.fillWidth: true
                placeholderText: "Xác nhận mật khẩu"
                echoMode: TextInput.Password
                
                onTextChanged: {
                    if (text.length > 0 && text !== passwordField.text) {
                        hasError = true
                        errorMessage = "Mật khẩu không khớp"
                    } else {
                        hasError = false
                    }
                }
            }
        }
        
        // Submit Button
        Button {
            id: submitButton
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            Layout.topMargin: 32
            Layout.leftMargin: 24
            Layout.rightMargin: 24
            Layout.bottomMargin: 24
            text: "HOÀN TẤT"
            font.family: "Lexend"
            font.pixelSize: 16
            font.weight: Font.DemiBold
            enabled: usernameField.text.length > 0 &&
                    passwordField.text.length > 0 &&
                    confirmPasswordField.text === passwordField.text &&
                    !confirmPasswordField.hasError
            
            opacity: enabled ? 1.0 : 0.5
            
            background: Rectangle {
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#673AB7" }
                    GradientStop { position: 1.0; color: "#9C27B0" }
                }
                radius: 8
            }
            
            contentItem: Text {
                text: submitButton.text
                font: submitButton.font
                color: "#FFFFFF"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            
            onClicked: {
                // Validate and register
                handleRegister()
            }
        }
    }
    
    // Toast Message Component
    ToastMessage {
        id: toastMessage
        anchors.fill: parent
    }
    
    // Timer for navigation after successful registration
    Timer {
        id: successTimer
        interval: 1500
        onTriggered: {
            if (stackView) {
                stackView.pop()
            }
        }
    }
    
    // Connect to NetworkClient signals
    Connections {
        target: networkClient
        
        function onRegisterResponse(success, error) {
            if (success) {
                toastMessage.show("Đăng ký thành công!", "#4CAF50")
                successTimer.start()
            } else {
                var errorMsg = "Đăng ký thất bại"
                if (error === "USERNAME_EXISTS") {
                    errorMsg = "Tên đăng nhập đã tồn tại"
                } else if (error === "REGISTER_FAILED") {
                    errorMsg = "Lỗi đăng ký. Vui lòng thử lại."
                } else if (error) {
                    errorMsg = "Lỗi: " + error
                }
                toastMessage.show(errorMsg, "#FF5252")
            }
        }
        
        function onErrorOccurred(error) {
            toastMessage.show("Lỗi: " + error, "#FF5252")
        }
    }
    
    function handleRegister() {
        // Validate all fields
        if (usernameField.text.length === 0) {
            toastMessage.show("Vui lòng nhập tên tài khoản", "#FF5252")
            return
        }
        
        if (passwordField.text.length === 0) {
            toastMessage.show("Vui lòng nhập mật khẩu", "#FF5252")
            return
        }
        
        if (confirmPasswordField.text !== passwordField.text) {
            toastMessage.show("Mật khẩu xác nhận không khớp", "#FF5252")
            return
        }
        
        // Connect to server if not connected
        if (!networkClient.isConnected()) {
            if (!networkClient.connectToServer("localhost", 9000)) {
                toastMessage.show("Không thể kết nối đến server", "#FF5252")
                return
            }
        }
        
        // Call NetworkClient register
        networkClient.sendRegister(usernameField.text.trim(), passwordField.text)
    }
}

