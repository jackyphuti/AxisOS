import QtQuick
import Qt5Compat.GraphicalEffects

Item {
    id: root
    anchors.fill: parent

    property bool isDarkMode: Theme.isDark
    property bool blurRequested: false

    // Unified Base Layer
    Rectangle {
        anchors.fill: parent
        color: Theme.base
    }

    // High-res AxisOS Signature Wallpaper
    Image {
        id: bgImage
        anchors.fill: parent
        source: "file:///usr/share/backgrounds/axisos/01-axisos-signature-core.png"
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        visible: !root.blurRequested
    }

    FastBlur {
        id: blurEffect
        anchors.fill: bgImage
        source: bgImage
        radius: 48
        visible: root.blurRequested
    }

    // Frosted glass overlay when menu/launcher is open
    Rectangle {
        anchors.fill: parent
        color: root.blurRequested ? Theme.glassTint : "transparent"
        Behavior on color {
            ColorAnimation { duration: 200 }
        }
    }
}
