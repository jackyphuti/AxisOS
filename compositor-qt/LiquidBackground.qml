import QtQuick

Item {
    id: root
    anchors.fill: parent

    property bool isDarkMode: false

    // One UI Soft Gradient Wallpaper
    // Light: 160deg #e9eefc -> #f6f1fb | Dark: 160deg #141826 -> #1b1d2b
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop {
                position: 0.0
                color: root.isDarkMode ? "#141826" : "#E9EEFC"
            }
            GradientStop {
                position: 1.0
                color: root.isDarkMode ? "#1B1D2B" : "#F6F1FB"
            }
        }
    }
}
