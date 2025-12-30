import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: friendsList
    width: parent ? parent.width : 600
    height: parent ? parent.height : 700
    
    property StackView stackView
    property string username: ""
    
    
    // Friends data from server
    property var allFriends: []
    property var filteredFriends: []
    property var pendingRequests: []  // Pending friend requests
    property bool showFriendsOnly: true
    property bool showPendingRequests: false  // Tab state
    
    // Connect to global NetworkClient instance (registered in main.cpp)
    Connections {
        target: networkClient
        
        function onListFriendsResult(friends) {
            console.log("=== FriendsList: listFriendsResult ===")
            console.log("Friends count:", friends.length)
            
            var tempList = []
            for (var i = 0; i < friends.length; i++) {
                var friend = friends[i]
                var userId = friend.user_id || friend.userId || 0
                var username = friend.username || ""
                var onlineStatus = friend.online_status || friend.onlineStatus || "offline"
                var isOnline = (onlineStatus === "online" || onlineStatus === "in_game")
                
                tempList.push({
                    id: userId,
                    userId: userId,
                    username: username,
                    online: isOnline,
                    onlineStatus: onlineStatus,
                    isFriend: true,
                    hasUnreadMessages: false,  // Badge indicator for unread messages
                    isAdding: false,  // Track if friend request is being sent
                    isAdded: false  // Track if friend request was sent (show checkmark)
                })
            }
            allFriends = tempList
            filterFriends(searchTextField.text)
        }
        
        function onSearchUserResult(users) {
            console.log("=== FriendsList: searchUserResult ===")
            console.log("Users count:", users.length)
            
            // Add search results to allFriends (mark as not friend)
            var tempList = allFriends.slice()  // Copy existing friends
            
            for (var i = 0; i < users.length; i++) {
                var user = users[i]
                var userId = user.user_id || user.userId || 0
                var username = user.username || ""
                
                // Check if already in list
                var exists = false
                for (var j = 0; j < tempList.length; j++) {
                    if (tempList[j].id === userId || tempList[j].userId === userId) {
                        exists = true
                        break
                    }
                }
                
                if (!exists) {
                    tempList.push({
                        id: userId,
                        userId: userId,
                        username: username,
                        online: false,
                        onlineStatus: "offline",
                        isFriend: false,
                        hasUnreadMessages: false,
                        isAdding: false,  // Track if friend request is being sent
                        isAdded: false  // Track if friend request was sent (show checkmark)
                    })
                }
            }
            
            allFriends = tempList
            filterFriends(searchTextField.text)
        }
        
        function onAddFriendResult(success, error) {
            console.log("=== FriendsList: addFriendResult ===")
            console.log("Success:", success, "Error:", error)
            
            // Find the user that was being added and update their state
            // We'll track this by finding users with isAdding = true
            var updated = false
            for (var i = 0; i < allFriends.length; i++) {
                if (allFriends[i].isAdding) {
                    if (success) {
                        // Set isAdded to true and reset isAdding
                        allFriends[i].isAdded = true
                        allFriends[i].isAdding = false
                    } else {
                        // Reset isAdding on failure
                        allFriends[i].isAdding = false
                    }
                    updated = true
                }
            }
            
            if (updated) {
                var newList = allFriends.slice()
                allFriends = newList
                filterFriends(searchTextField.text)
            }
            
            if (success) {
                console.log("Add friend successful")
                // Refresh friends list after a delay to update friend status
                // (in case the friend accepts immediately)
                refreshTimer.start()
            } else {
                console.log("Add friend failed:", error)
            }
        }
        
        function onRemoveFriendResult(success, error) {
            console.log("=== FriendsList: removeFriendResult ===")
            console.log("Success:", success, "Error:", error)
            
            if (success) {
                // Refresh friends list after successful removal
                var nc = networkClient
                if (nc) {
                    nc.sendListFriends()
                }
            } else {
                console.log("Failed to remove friend:", error)
            }
        }
        
        function onFriendStatusChanged(userId, status, roomId) {
            console.log("=== FriendsList: friendStatusChanged ===")
            console.log("UserId:", userId, "Status:", status)
            
            // Check if friend exists in list
            var found = false
            for (var i = 0; i < allFriends.length; i++) {
                if (allFriends[i].id === userId || allFriends[i].userId === userId) {
                    // Friend exists, update status
                    allFriends[i].online = (status === "online" || status === "in_game")
                    allFriends[i].onlineStatus = status
                    allFriends = allFriends.slice()  // Force update
                    filterFriends(searchTextField.text)
                    found = true
                    break
                }
            }
            
            // If friend not found in list, refresh friends list from server
            // This happens when B accepts A's request - A receives notification but B is not in A's list yet
            if (!found) {
                console.log("Friend not found in list, refreshing from server...")
                if (networkClient) {
                    networkClient.sendListFriends()
                }
            }
        }
        
        function onDmReceived(fromUserId, fromUsername, message, timestamp) {
            console.log("=== FriendsList: dmReceived ===")
            console.log("From:", fromUserId, "Message:", message)
            
            // Set unread badge for friend (only if not currently in ChatScreen with this friend)
            var updated = false
            for (var i = 0; i < allFriends.length; i++) {
                if (allFriends[i].id === fromUserId || allFriends[i].userId === fromUserId) {
                    allFriends[i].hasUnreadMessages = true
                    updated = true
                    console.log("Set unread badge for friend:", fromUserId)
                    break
                }
            }
            
            // Force update by creating new array reference
            if (updated) {
                var newList = allFriends.slice()
                allFriends = newList
                filterFriends(searchTextField.text)
            }
        }
        
        function onPendingRequestsReceived(requests) {
            console.log("=== FriendsList: pendingRequestsReceived ===")
            console.log("Requests count:", requests.length)
            console.log("Requests data:", JSON.stringify(requests))
            
            var tempList = []
            for (var i = 0; i < requests.length; i++) {
                var req = requests[i]
                var userId = req.user_id || req.userId || 0
                var username = req.username || ""
                
                console.log("Pending request:", userId, username)
                
                tempList.push({
                    id: userId,
                    userId: userId,
                    username: username
                })
            }
            pendingRequests = tempList
            console.log("Updated pendingRequests, count:", pendingRequests.length)
        }
        
        function onRespondFriendResult(success, error) {
            console.log("=== FriendsList: respondFriendResult ===")
            console.log("Success:", success, "Error:", error)
            
            if (success) {
                // Refresh both friends list and pending requests
                var nc = networkClient
                if (nc) {
                    nc.sendListFriends()
                    nc.sendGetPendingRequests()
                }
            } else {
                console.log("Failed to respond to friend request:", error)
            }
        }
        
        function onFriendRequestReceived(fromUserId, fromUsername) {
            console.log("=== FriendsList: friendRequestReceived ===")
            console.log("From:", fromUserId, fromUsername)
            
            // Add to pending requests if not already there
            var exists = false
            for (var i = 0; i < pendingRequests.length; i++) {
                if (pendingRequests[i].id === fromUserId || pendingRequests[i].userId === fromUserId) {
                    exists = true
                    break
                }
            }
            
            if (!exists) {
                console.log("Adding new friend request to pendingRequests")
                var newList = pendingRequests.slice()
                newList.push({
                    id: fromUserId,
                    userId: fromUserId,
                    username: fromUsername
                })
                pendingRequests = newList
                console.log("Updated pendingRequests, count:", pendingRequests.length)
            } else {
                console.log("Friend request already in pendingRequests")
            }
        }
    }
    
    // Timer to refresh friends list after adding friend
    Timer {
        id: refreshTimer
        interval: 1000  // 1 second delay
        onTriggered: {
            networkClient.sendListFriends()
        }
    }
    
    // Confirmation dialog for deleting friend
    property var friendToDelete: null  // Store friend info when delete button is clicked
    
    Dialog {
        id: deleteConfirmDialog
        title: "Xác nhận xóa bạn bè"
        anchors.centerIn: parent
        width: 400
        height: 200
        modal: true
        
        background: Rectangle {
            color: "#2E1A47"
            radius: 10
            border.color: "#FF5252"
            border.width: 2
        }
        
        contentItem: ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 20
            
            Text {
                Layout.fillWidth: true
                text: friendToDelete ? "Bạn có chắc chắn muốn xóa " + friendToDelete.username + " khỏi danh sách bạn bè không?" : ""
                font.family: "Lexend"
                font.pixelSize: 16
                color: "#FFFFFF"
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
            
            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                spacing: 15
                
                Button {
                    Layout.fillWidth: true
                    text: "Hủy"
                    font.family: "Lexend"
                    font.pixelSize: 14
                    
                    background: Rectangle {
                        color: parent.hovered ? Qt.rgba(1.0, 1.0, 1.0, 0.2) : Qt.rgba(1.0, 1.0, 1.0, 0.1)
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
                        deleteConfirmDialog.close()
                        friendToDelete = null
                    }
                }
                
                Button {
                    Layout.fillWidth: true
                    text: "Xóa"
                    font.family: "Lexend"
                    font.pixelSize: 14
                    
                    background: Rectangle {
                        color: parent.hovered ? "#FF5252" : "#FF6B6B"
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
                        if (friendToDelete) {
                            var friendId = friendToDelete.id || friendToDelete.userId || 0
                            console.log("Confirming removal of friend:", friendId)
                            networkClient.sendRemoveFriend(friendId)
                            deleteConfirmDialog.close()
                            friendToDelete = null
                        }
                    }
                }
            }
        }
    }
    
    Component.onCompleted: {
        // Load friends list and pending requests when screen is shown
        var nc = networkClient
        if (nc) {
            nc.sendListFriends()
            nc.sendGetPendingRequests()
        }
    }
    
    // Reload when screen becomes visible (when navigating back from other screens)
    onVisibleChanged: {
        if (visible) {
            console.log("FriendsList became visible, reloading friends list...")
            var nc = networkClient
            if (nc) {
                nc.sendListFriends()
                nc.sendGetPendingRequests()
            }
        }
    }
    
    // Background
    Rectangle {
        anchors.fill: parent
        color: "#2E1A47"
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15
        
        // Header
        RowLayout {
            Layout.fillWidth: true
            
                Button {
                    text: "←"
                    font.pixelSize: 20
                    flat: true
                    onClicked: {
                        console.log("FriendsList back button clicked")
                        console.log("stackView:", stackView)
                        console.log("StackView depth:", stackView ? stackView.depth : "null")
                        if (stackView) {
                            var result = stackView.pop()
                            console.log("pop() result:", result)
                        } else {
                            console.log("ERROR: stackView is null!")
                        }
                    }
                
                contentItem: Text {
                    text: parent.text
                    font: parent.font
                    color: "#FFFFFF"
                }
            }
            
            Text {
                Layout.fillWidth: true
                text: "Danh sách bạn bè"
                font.family: "Lexend"
                font.pixelSize: 20
                font.bold: true
                color: "#FFFFFF"
                horizontalAlignment: Text.AlignHCenter
            }
            
            Item {
                width: 40
            }
        }
        
        // Search bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            radius: 10
            color: Qt.rgba(1.0, 1.0, 1.0, 0.1)
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10
                
                Text {
                    text: "🔍"
                    font.pixelSize: 18
                }
                
                TextField {
                    id: searchTextField
                    Layout.fillWidth: true
                    placeholderText: "Tìm kiếm bạn bè..."
                    font.family: "Lexend"
                    font.pixelSize: 14
                    color: "#FFFFFF"
                    background: Rectangle {
                        color: "transparent"
                    }
                    
                    onTextChanged: {
                        if (text.length > 0) {
                            // Search users
                            networkClient.sendSearchUser(text, 20)
                        } else {
                            // Show friends only
                            filterFriends("")
                        }
                    }
                }
            }
        }
        
        // Tabs
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            
            Button {
                Layout.fillWidth: true
                text: "Bạn bè"
                font.family: "Lexend"
                font.pixelSize: 14
                
                background: Rectangle {
                    color: (!showPendingRequests && showFriendsOnly) ? "#FFC107" : Qt.rgba(1.0, 1.0, 1.0, 0.1)
                    radius: 8
                }
                
                contentItem: Text {
                    text: parent.text
                    font: parent.font
                    color: (!showPendingRequests && showFriendsOnly) ? "#000000" : "#FFFFFF"
                    horizontalAlignment: Text.AlignHCenter
                }
                
                onClicked: {
                    showPendingRequests = false
                    showFriendsOnly = true
                    filterFriends(searchTextField.text)
                }
            }
            
            Button {
                Layout.fillWidth: true
                text: "Yêu cầu kết bạn" + (pendingRequests.length > 0 ? " (" + pendingRequests.length + ")" : "")
                font.family: "Lexend"
                font.pixelSize: 14
                
                background: Rectangle {
                    color: showPendingRequests ? "#FFC107" : Qt.rgba(1.0, 1.0, 1.0, 0.1)
                    radius: 8
                }
                
                contentItem: Text {
                    text: parent.text
                    font: parent.font
                    color: showPendingRequests ? "#000000" : "#FFFFFF"
                    horizontalAlignment: Text.AlignHCenter
                }
                
                onClicked: {
                    showPendingRequests = true
                    var nc = networkClient
                    if (nc) {
                        nc.sendGetPendingRequests()
                    }
                }
            }
            
            Button {
                Layout.fillWidth: true
                text: "Tìm kiếm"
                font.family: "Lexend"
                font.pixelSize: 14
                
                background: Rectangle {
                    color: (!showPendingRequests && !showFriendsOnly) ? "#FFC107" : Qt.rgba(1.0, 1.0, 1.0, 0.1)
                    radius: 8
                }
                
                contentItem: Text {
                    text: parent.text
                    font: parent.font
                    color: (!showPendingRequests && !showFriendsOnly) ? "#000000" : "#FFFFFF"
                    horizontalAlignment: Text.AlignHCenter
                }
                
                onClicked: {
                    showPendingRequests = false
                    showFriendsOnly = false
                    filterFriends(searchTextField.text)
                }
            }
        }
        
        // Pending requests list (when showPendingRequests is true)
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            visible: showPendingRequests
            
            ListView {
                id: pendingRequestsListView
                width: parent.width
                model: pendingRequests
                spacing: 10
                
                delegate: Rectangle {
                    width: pendingRequestsListView.width
                    height: 70
                    color: Qt.rgba(1.0, 1.0, 1.0, 0.1)
                    radius: 10
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 15
                        
                        Text {
                            Layout.fillWidth: true
                            text: modelData.username || ""
                            font.family: "Lexend"
                            font.pixelSize: 16
                            color: "#FFFFFF"
                            elide: Text.ElideRight
                        }
                        
                        Button {
                            Layout.preferredWidth: 80
                            Layout.preferredHeight: 35
                            text: "Chấp nhận"
                            font.family: "Lexend"
                            font.pixelSize: 12
                            
                            background: Rectangle {
                                color: parent.hovered ? "#4CAF50" : "#66BB6A"
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
                                var fromUserId = modelData.id || modelData.userId || 0
                                console.log("Accepting friend request from:", fromUserId)
                                
                                // Gọi function ở root level
                                friendsList.sendRespondFriendRequest(fromUserId, true)
                            }
                        }
                        
                        Button {
                            Layout.preferredWidth: 80
                            Layout.preferredHeight: 35
                            text: "Từ chối"
                            font.family: "Lexend"
                            font.pixelSize: 12
                            
                            background: Rectangle {
                                color: parent.hovered ? "#FF5252" : "#EF5350"
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
                                var fromUserId = modelData.id || modelData.userId || 0
                                console.log("Rejecting friend request from:", fromUserId)
                                
                                // Gọi function ở root level
                                friendsList.sendRespondFriendRequest(fromUserId, false)
                            }
                        }
                    }
                }
            }
        }
        
        // Friends/Users list (when showPendingRequests is false)
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            visible: !showPendingRequests
            
            ListView {
                id: friendsListView
                width: parent.width
                model: filteredFriends
                spacing: 10
                
                delegate: Rectangle {
                    width: friendsListView.width
                    height: 60
                    color: Qt.rgba(1.0, 1.0, 1.0, 0.1)
                    radius: 10
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 15
                        
                        // Online indicator
                        Rectangle {
                            Layout.preferredWidth: 12
                            Layout.preferredHeight: 12
                            radius: 6
                            color: modelData.online ? "#4CAF50" : "#CCCCCC"
                        }
                        
                        Text {
                            Layout.fillWidth: true
                            text: modelData.username
                            font.family: "Lexend"
                            font.pixelSize: 16
                            color: "#FFFFFF"
                            elide: Text.ElideRight
                        }
                        
                        // Unread message badge "!" (only for friends)
                        Rectangle {
                            Layout.preferredWidth: 20
                            Layout.preferredHeight: 20
                            Layout.alignment: Qt.AlignVCenter
                            radius: 10
                            color: "#FF5252"
                            visible: (modelData.isFriend && (modelData.hasUnreadMessages || false))
                            
                            // Disable mouse events so clicks pass through to parent
                            enabled: false
                            
                            Text {
                                anchors.centerIn: parent
                                text: "!"
                                font.family: "Lexend"
                                font.pixelSize: 12
                                font.bold: true
                                color: "#FFFFFF"
                            }
                        }
                        
                        // Delete friend button (only for friends)
                        Button {
                            Layout.preferredWidth: 40
                            Layout.preferredHeight: 40
                            Layout.alignment: Qt.AlignVCenter
                            text: "🗑️"
                            font.pixelSize: 16
                            flat: true
                            visible: modelData.isFriend
                            z: 10  // Ensure button is above MouseArea
                            
                            background: Rectangle {
                                color: parent.hovered ? Qt.rgba(1.0, 0.0, 0.0, 0.2) : "transparent"
                                radius: 8
                            }
                            
                            onClicked: {
                                // Store friend info and show confirmation dialog
                                friendToDelete = {
                                    id: modelData.id || modelData.userId || 0,
                                    userId: modelData.userId || modelData.id || 0,
                                    username: modelData.username || ""
                                }
                                console.log("Delete button clicked for friend:", friendToDelete.username)
                                deleteConfirmDialog.open()
                            }
                        }
                        
                        // Add friend button (for non-friends)
                        Button {
                            Layout.preferredWidth: 40
                            Layout.preferredHeight: 40
                            Layout.alignment: Qt.AlignVCenter
                            text: {
                                if (modelData.isAdded) return "✓"
                                if (modelData.isAdding) return "⏳"
                                return "➕"
                            }
                            font.pixelSize: 18
                            flat: true
                            visible: !modelData.isFriend
                            z: 10  // Ensure button is above MouseArea
                            
                            background: Rectangle {
                                color: {
                                    if (modelData.isAdded) return Qt.rgba(0.0, 1.0, 0.0, 0.2)
                                    if (parent.hovered) return Qt.rgba(1.0, 1.0, 1.0, 0.2)
                                    return "transparent"
                                }
                                radius: 8
                            }
                            
                            onClicked: {
                                if (modelData.isAdded || modelData.isAdding) {
                                    // Already added or adding, do nothing
                                    return
                                }
                                
                                var friendId = modelData.id || modelData.userId || 0
                                console.log("Add friend:", modelData.username, "ID:", friendId)
                                
                                // Gọi function ở root level (giống ChatScreen.sendMessage)
                                // Sử dụng ID của root Item trực tiếp
                                friendsList.sendAddFriendRequest(friendId)
                            }
                        }
                    }
                    
                    // MouseArea only for the text area, not covering buttons
                    // We'll use a MouseArea that doesn't cover the right side where buttons are
                    MouseArea {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.rightMargin: 80  // Leave space for buttons on the right (only delete/add button now)
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        z: 0  // Ensure MouseArea is below buttons
                        onClicked: {
                            // Only open chat if clicking on the row itself, not on buttons
                            if (modelData.isFriend) {
                                var friendId = modelData.id || modelData.userId || 0
                                
                                // Reset unread badge immediately (before push to avoid delay)
                                for (var i = 0; i < allFriends.length; i++) {
                                    if (allFriends[i].id === friendId || allFriends[i].userId === friendId) {
                                        allFriends[i].hasUnreadMessages = false
                                        var newList = allFriends.slice()
                                        allFriends = newList
                                        filterFriends(searchTextField.text)
                                        break
                                    }
                                }
                                
                                if (stackView) {
                                    stackView.push("ChatScreen.qml", {
                                        "stackView": stackView,
                                        "username": username,
                                        "friendId": friendId,
                                        "friendName": modelData.username
                                    })
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    function filterFriends(searchText) {
        var filtered = []
        for (var i = 0; i < allFriends.length; i++) {
            var friend = allFriends[i]
            var matchesSearch = searchText === "" || friend.username.toLowerCase().includes(searchText.toLowerCase())
            var matchesFilter = !showFriendsOnly || friend.isFriend
            
            if (matchesSearch && matchesFilter) {
                filtered.push(friend)
            }
        }
        filteredFriends = filtered
    }
    
    // Functions to handle friend requests (called from delegates)
    function sendAddFriendRequest(friendId) {
        console.log("=== sendAddFriendRequest ===")
        console.log("friendId:", friendId)
        
        // Update UI immediately (optimistic update)
        for (var i = 0; i < allFriends.length; i++) {
            if (allFriends[i].id === friendId || allFriends[i].userId === friendId) {
                allFriends[i].isAdding = true
                var newList = allFriends.slice()
                allFriends = newList
                filterFriends(searchTextField.text)
                break
            }
        }
        
        // Send to server
        if (networkClient) {
            networkClient.sendAddFriend(friendId)
        } else {
            console.error("networkClient is not available")
        }
    }
    
    function sendRespondFriendRequest(fromUserId, accept) {
        console.log("=== sendRespondFriendRequest ===")
        console.log("fromUserId:", fromUserId, "accept:", accept)
        
        // Send to server
        if (networkClient) {
            networkClient.sendRespondFriend(fromUserId, accept)
        } else {
            console.error("networkClient is not available")
        }
    }
}

