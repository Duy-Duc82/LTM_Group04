import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "components"

ScrollView {
    id: signinScroll
    width: parent ? parent.width : 400
    height: parent ? parent.height : 700
    clip: true
    
    property StackView stackView
    
    ColumnLayout {
        id: signinLayout
        width: parent.width - 48 // 24px padding each side
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 0
        
        // Header Section (Top 25%)
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 200
            Layout.topMargin: 50
            
            // Logo
            Rectangle {
                id: signinLogo
                width: 120
                height: 120
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                color: "transparent"
                
                // Placeholder for logo
                Text {
                    anchors.centerIn: parent
                    text: "💰"
                    font.pixelSize: 60
                }
            }
            
            // Welcome Text
            Text {
                id: welcomeText
                text: "Chào mừng bạn!"
                font.family: "Lexend"
                font.pixelSize: 28
                font.bold: true
                color: "#FFFFFF"
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: signinLogo.bottom
                anchors.topMargin: 24
            }
        }
        
        // Server Connection Section
        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: 20
            Layout.leftMargin: 24
            Layout.rightMargin: 24
            spacing: 8
            
            Text {
                text: "Kết nối Server"
                font.family: "Lexend"
                font.pixelSize: 14
                color: "#FFFFFF"
            }
            
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                
                CustomTextField {
                    id: serverHostField
                    Layout.fillWidth: true
                    Layout.preferredWidth: 200
                    placeholderText: "Host (localhost)"
                    text: "localhost"
                    enabled: !networkClient.isConnected()
                }
                
                CustomTextField {
                    id: serverPortField
                    Layout.preferredWidth: 80
                    placeholderText: "Port"
                    text: "9000"
                    enabled: !networkClient.isConnected()
                }
                
                Button {
                    id: connectButton
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 48
                    
                    // Use property to track connection state for reliable updates
                    property bool isConnected: false
                    text: isConnected ? "Ngắt kết nối" : "Kết nối"
                    font.family: "Lexend"
                    font.pixelSize: 14
                    
                    background: Rectangle {
                        color: connectButton.isConnected ? "#FF5252" : "#FFC107"
                        radius: 8
                    }
                    
                    contentItem: Text {
                        text: connectButton.text
                        font: connectButton.font
                        color: "#000000"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    // Update connection state when signals are received
                    Connections {
                        target: networkClient
                        function onConnected() {
                            connectButton.isConnected = true
                            console.log("Connect button: Connection state updated to true")
                        }
                        function onDisconnected() {
                            connectButton.isConnected = false
                            console.log("Connect button: Connection state updated to false")
                        }
                    }
                    
                    // Initialize connection state
                    Component.onCompleted: {
                        isConnected = networkClient ? networkClient.isConnected() : false
                    }
                    
                    onClicked: {
                        if (isConnected) {
                            // Disconnect
                            networkClient.disconnectFromServer()
                            toastMessage.show("Đang ngắt kết nối...", "#FFC107")
                        } else {
                            // Connect
                            var host = serverHostField.text || "localhost"
                            var port = parseInt(serverPortField.text) || 9000
                            console.log("Attempting to connect to", host, ":", port)
                            if (networkClient.connectToServer(host, port)) {
                                toastMessage.show("Đang kết nối...", "#FFC107")
                            } else {
                                // Error message will be shown via errorOccurred signal
                                console.log("Connection failed")
                            }
                        }
                    }
                }
            }
        }
        
        // Form Section (Center)
        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: 40
            Layout.leftMargin: 24
            Layout.rightMargin: 24
            spacing: 16
            
            // Username/Email Field
            CustomTextField {
                id: usernameField
                Layout.fillWidth: true
                placeholderText: "Tên đăng nhập hoặc Email"
            }
            
            // Password Field - NO VALIDATION, accept any password
            CustomTextField {
                id: passwordField
                Layout.fillWidth: true
                placeholderText: "Mật khẩu"
                echoMode: TextInput.Password
                
                // Log password changes for debugging
                onTextChanged: {
                    console.log("Password field changed, length:", text.length, "text:", text)
                }
            }
        }
        
        // Action & Social Section (Bottom)
        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: 32
            Layout.leftMargin: 24
            Layout.rightMargin: 24
            Layout.bottomMargin: 24
            spacing: 24
            
            // Login Button
            Button {
                id: loginButton
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                text: "ĐĂNG NHẬP"
                font.family: "Lexend"
                font.pixelSize: 16
                font.weight: Font.DemiBold
                
                background: Rectangle {
                    color: loginButton.enabled ? "#FFC107" : "#A0A0A0"
                    radius: 8
                }
                
                contentItem: Text {
                    text: loginButton.text
                    font: loginButton.font
                    color: "#000000"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                // Use property to track connection state
                property bool isServerConnected: false
                enabled: isServerConnected
                opacity: enabled ? 1.0 : 0.5
                
                // Force update when connection changes
                Connections {
                    target: networkClient
                    function onConnected() {
                        console.log("=== CONNECTION SIGNAL RECEIVED IN BUTTON ===")
                        console.log("Setting isServerConnected to true")
                        loginButton.isServerConnected = true
                        console.log("Button enabled after update:", loginButton.enabled)
                    }
                    function onDisconnected() {
                        console.log("=== DISCONNECTION SIGNAL RECEIVED IN BUTTON ===")
                        loginButton.isServerConnected = false
                    }
                }
                
                // Debug: Log button state changes
                Component.onCompleted: {
                    console.log("=== LOGIN BUTTON CREATED ===")
                    console.log("Initial enabled state:", enabled)
                    console.log("Initial isConnected:", networkClient ? networkClient.isConnected() : "networkClient is null")
                    console.log("isServerConnected property:", isServerConnected)
                }
                
                onEnabledChanged: {
                    console.log("=== LOGIN BUTTON STATE CHANGED ===")
                    console.log("Enabled:", enabled)
                    console.log("isServerConnected:", isServerConnected)
                    console.log("Is connected (direct call):", networkClient ? networkClient.isConnected() : "null")
                    console.log("Username:", usernameField.text)
                    console.log("Password length:", passwordField.text.length)
                }
                
                // Also log when button is pressed (even if disabled)
                onPressed: {
                    console.log("=== LOGIN BUTTON PRESSED ===")
                    console.log("Enabled:", enabled)
                    console.log("Is connected:", networkClient.isConnected())
                }
                
                onClicked: {
                    console.log("=== LOGIN BUTTON CLICKED ===")
                    console.log("Button enabled:", loginButton.enabled)
                    console.log("Is connected:", networkClient.isConnected())
                    console.log("Username field text:", usernameField.text)
                    console.log("Password field text:", passwordField.text)
                    console.log("Password field text length:", passwordField.text.length)
                    console.log("Password field text type:", typeof passwordField.text)
                    
                    // Check if button is enabled
                    if (!loginButton.enabled) {
                        console.log("ERROR: Button is disabled - cannot login without connection")
                        toastMessage.show("Vui lòng kết nối đến server trước", "#FF5252")
                        return
                    }
                    
                    // ONLY check if fields are empty - NO password length validation
                    if (usernameField.text.length === 0 || passwordField.text.length === 0) {
                        console.log("Validation failed: empty fields")
                        toastMessage.show("Vui lòng điền đầy đủ thông tin", "#FF5252")
                        return
                    }
                    
                    console.log("Fields validated, calling handleLogin...")
                    // Call login function - pass password AS-IS, no modification
                    handleLogin(usernameField.text, passwordField.text)
                }
            }
            
            // Footer Link - Use RowLayout with Text and MouseArea for better clickability
            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                spacing: 4
                
                Text {
                    text: "Chưa có tài khoản?"
                    font.family: "Lexend"
                    font.pixelSize: 14
                    color: "#FFFFFF"
                }
                
                Text {
                    id: signupLink
                    text: "Đăng ký ngay"
                    font.family: "Lexend"
                    font.pixelSize: 14
                    color: "#FFC107"
                    font.underline: true
                    
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            console.log("Signup link clicked")
                            if (stackView) {
                                stackView.push("Signup.qml", {"stackView": stackView})
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Toast Message Component
    ToastMessage {
        id: toastMessage
        anchors.fill: parent
    }
    
    // Connect to NetworkClient signals
    Connections {
        target: networkClient
        
        function onConnected() {
            console.log("=== NETWORK CLIENT CONNECTED SIGNAL (Main Connections) ===")
            console.log("isConnected() returns:", networkClient.isConnected())
            
            // Update connect button state
            connectButton.isConnected = true
            
            // Update login button state
            console.log("Login button enabled state (before):", loginButton.enabled)
            console.log("Login button isServerConnected (before):", loginButton.isServerConnected)
            loginButton.isServerConnected = true
            console.log("Login button isServerConnected (after):", loginButton.isServerConnected)
            console.log("Login button enabled state (after):", loginButton.enabled)
            
            toastMessage.show("Đã kết nối đến server", "#4CAF50")
        }
        
        function onDisconnected() {
            // Update connect button state
            connectButton.isConnected = false
            
            toastMessage.show("Đã ngắt kết nối với server", "#FF5252")
        }
        
        function onLoginResponse(success, token, error) {
            console.log("=== onLoginResponse SIGNAL RECEIVED ===")
            console.log("Success:", success, "Token:", token, "Error:", error)
            console.log("stackView exists:", stackView !== null && stackView !== undefined)
            
            if (success) {
                console.log("Login successful, navigating to game mode selection")
                console.log("Username:", usernameField.text.trim())
                toastMessage.show("Đăng nhập thành công!", "#4CAF50")
                
                // Navigate to home screen
                if (stackView) {
                    console.log("stackView is valid, calling replace...")
                    try {
                        stackView.replace("HomeScreen.qml", {
                            "stackView": stackView,
                            "username": usernameField.text.trim()
                        })
                        console.log("Navigation successful")
                    } catch (err) {
                        console.log("ERROR during navigation:", err)
                        toastMessage.show("Lỗi khi chuyển màn hình: " + err, "#FF5252")
                    }
                } else {
                    console.log("ERROR: stackView is null or undefined!")
                    toastMessage.show("Lỗi: Không thể chuyển màn hình", "#FF5252")
                }
            } else {
                var errorMsg = error || "Sai thông tin đăng nhập"
                console.log("Login failed:", errorMsg)
                toastMessage.show("Đăng nhập thất bại: " + errorMsg, "#FF5252")
            }
        }
        
        function onErrorOccurred(error) {
            toastMessage.show("Lỗi: " + error, "#FF5252")
        }
    }
    
    // Functions
    function handleLogin(username, password) {
        console.log("=== handleLogin CALLED ===")
        console.log("Username (raw):", username, "type:", typeof username)
        console.log("Password (raw):", password, "type:", typeof password)
        console.log("Username length:", username.length)
        console.log("Password length:", password.length)
        
        if (!networkClient.isConnected()) {
            console.log("ERROR: Not connected to server")
            toastMessage.show("Vui lòng kết nối đến server trước", "#FF5252")
            return
        }
        
        // ONLY check if fields are not empty - NO password length/strength validation
        if (username.length === 0 || password.length === 0) {
            console.log("ERROR: Empty fields")
            toastMessage.show("Vui lòng điền đầy đủ thông tin", "#FF5252")
            return
        }
        
        // Trim username but keep password EXACTLY as entered
        var trimmedUsername = username.trim()
        var passwordToSend = String(password)  // Ensure it's a string, no modification
        
        console.log("=== FINAL VALUES TO SEND ===")
        console.log("Username (trimmed):", trimmedUsername)
        console.log("Password (as-is):", passwordToSend)
        console.log("Password length:", passwordToSend.length)
        console.log("Password char codes:", passwordToSend.split('').map(c => c.charCodeAt(0)))
        console.log("Calling networkClient.sendLogin...")
        
        // Call NetworkClient login - send password EXACTLY as user typed it
        try {
            networkClient.sendLogin(trimmedUsername, passwordToSend)
            console.log("sendLogin called successfully")
        } catch (error) {
            console.log("ERROR calling sendLogin:", error)
            toastMessage.show("Lỗi khi gửi yêu cầu đăng nhập: " + error, "#FF5252")
        }
    }
}

