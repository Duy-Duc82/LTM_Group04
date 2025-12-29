import QtQuick 6.0

Row {
    id: strengthIndicator
    spacing: 4
    height: 4
    
    property string password: ""
    
    Repeater {
        model: 3
        
        Rectangle {
            width: 32
            height: 4
            radius: 2
            color: {
                var strength = calculateStrength(strengthIndicator.password)
                if (strength === 0) return "#FF5252" // Red
                if (strength === 1) return "#FFC107" // Yellow
                return "#4CAF50" // Green
            }
            opacity: index < calculateStrength(strengthIndicator.password) + 1 ? 1.0 : 0.3
        }
    }
    
    function calculateStrength(pwd) {
        if (pwd.length < 6) return 0 // Weak
        
        var lettersOnly = /^[a-zA-Z]+$/
        var hasLetters = /[a-zA-Z]/
        var hasNumbers = /[0-9]/
        var hasSpecial = /[^a-zA-Z0-9]/
        
        if (lettersOnly.test(pwd)) return 1 // Medium (letters only)
        if (hasLetters.test(pwd) && hasNumbers.test(pwd) && hasSpecial.test(pwd)) {
            return 2 // Strong
        }
        return 1 // Medium
    }
}

