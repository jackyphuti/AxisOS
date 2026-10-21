import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: osdRoot
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.bottom: parent.bottom
    anchors.bottomMargin: 84
    width: 250
    height: 46
    z: 9600

    property string osdType: "volume"
    property int osdValue: 65
    property string osdIcon: "🔊"
    property bool isVisible: false

    opacity: isVisible ? 1.0 : 0.0
    scale: isVisible ? 1.0 : 0.92

    Behavior on opacity {
        NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
    }
    Behavior on scale {
        SpringAnimation { spring: Theme.springConstant; damping: Theme.springDamping; epsilon: Theme.springEpsilon }
    }

    Connections {
        target: (typeof SystemController !== "undefined" && SystemController !== null) ? SystemController : null

        function onOsdTriggered(type, value, icon) {
            osdRoot.osdType = type;
            osdRoot.osdValue = Math.max(0, Math.min(100, value));
            if (icon.indexOf("mute") !== -1) osdRoot.osdIcon = "🔇";
            else if (type === "volume") osdRoot.osdIcon = value > 60 ? "🔊" : "🔉";
            else if (type === "brightness") osdRoot.osdIcon = "☀️";
            else osdRoot.osdIcon = "ℹ️";

            osdRoot.isVisible = true;
            hideTimer.restart();
        }
    }

    Timer {
        id: hideTimer
        interval: 1800
        repeat: false
        onTriggered: osdRoot.isVisible = false
    }

    // Outer shadow
    Rectangle {
        anchors.fill: osdCard
        anchors.topMargin: 4
        radius: Theme.radiusPill
        color: "transparent"
        border.color: Theme.shadowColor
        border.width: 8
        opacity: 0.7
        z: -1
    }

    // OSD Capsule Card
    Rectangle {
        id: osdCard
        anchors.fill: parent
        radius: Theme.radiusPill
        color: Theme.surface
        border.color: Theme.divider
        border.width: 1

        // 1px Inner Specular Border
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: Theme.radiusPill
            color: "transparent"
            border.color: Theme.specularBorder
            border.width: 1
            z: 10
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 12

            Text {
                text: osdRoot.osdIcon
                font.pixelSize: 15
                color: Theme.textPrimary
            }

            // Progress bar
            Rectangle {
                Layout.fillWidth: true
                height: 6
                radius: 3
                color: Theme.surfaceAlt

                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: Math.max(0, Math.min(parent.width, (osdRoot.osdValue / 100.0) * parent.width))
                    radius: 3
                    color: Theme.accent

                    Behavior on width {
                        SmoothedAnimation { velocity: 300 }
                    }
                }
            }

            Text {
                text: osdRoot.osdValue + "%"
                font.bold: true
                font.pixelSize: 11
                color: Theme.textPrimary
                Layout.preferredWidth: 32
                horizontalAlignment: Text.AlignRight
            }
        }
    }
}
