import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: topBar
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right
    height: 38
    color: "#FFFFFF"
    border.color: "#E5E7EE"
    border.width: 1

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 18
        anchors.rightMargin: 18
        spacing: 16

        // Left Branding & Launcher Trigger (Clean One UI Pill)
        RowLayout {
            spacing: 10

            Rectangle {
                id: brandBtn
                height: 28
                width: brandRow.implicitWidth + 16
                radius: 14
                color: brandMouse.containsMouse ? "#E6EBFF" : "#F4F6FB"
                border.color: brandMouse.containsMouse ? "#1428A0" : "#E5E7EE"
                border.width: 1

                Behavior on color { ColorAnimation { duration: 180; easing.type: Easing.OutCubic } }
                Behavior on border.color { ColorAnimation { duration: 180; easing.type: Easing.OutCubic } }

                RowLayout {
                    id: brandRow
                    anchors.centerIn: parent
                    spacing: 7

                    // Clean Samsung Deep Blue Accent Dot
                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: "#1428A0"
                    }

                    Text {
                        text: "AxisOS"
                        color: "#1C1C1E"
                        font.bold: true
                        font.pixelSize: 13
                        font.family: "Inter, -apple-system, system-ui, sans-serif"
                    }
                }

                MouseArea {
                    id: brandMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (typeof appLauncher !== "undefined") {
                            appLauncher.visible = !appLauncher.visible;
                        }
                    }
                }
            }
        }

        Item { Layout.fillWidth: true }

        // Center Date & Time Clock (Clean One UI typography)
        Text {
            id: clockDisplay
            color: "#1C1C1E"
            font.bold: true
            font.pixelSize: 13
            font.family: "Inter, -apple-system, system-ui, sans-serif"

            Timer {
                interval: 1000
                running: true
                repeat: true
                triggeredOnStart: true
                onTriggered: {
                    var now = new Date();
                    clockDisplay.text = Qt.formatDateTime(now, "ddd, MMM d   •   h:mm AP");
                }
            }
        }

        Item { Layout.fillWidth: true }

        // Right Telemetry & Status Area (Calm, clean outline indicators)
        RowLayout {
            spacing: 14

            // Focus pill (NeuralPulse) in subtle soft-blue pill
            Rectangle {
                height: 24
                width: focusRow.implicitWidth + 14
                radius: 12
                color: "#F4F6FB"
                border.color: "#E5E7EE"
                border.width: 1

                RowLayout {
                    id: focusRow
                    anchors.centerIn: parent
                    spacing: 6

                    Text {
                        text: SystemController.neuralPulseTarget
                        color: "#1C1C1E"
                        font.pixelSize: 11
                        font.bold: true
                        font.family: "Inter, -apple-system, system-ui, sans-serif"
                    }
                }
            }

            // Wi-Fi Status
            RowLayout {
                spacing: 5
                Rectangle {
                    width: 7
                    height: 7
                    radius: 3.5
                    color: SystemController.wifiConnected ? "#1428A0" : "#6B6F7B"
                }
                Text {
                    text: SystemController.wifiSsid
                    color: "#1C1C1E"
                    font.pixelSize: 12
                    font.family: "Inter, -apple-system, system-ui, sans-serif"
                }
            }

            // Battery Status
            RowLayout {
                spacing: 5
                Text {
                    text: (SystemController.batteryCharging ? "⚡ " : "") + SystemController.batteryPercent + "%"
                    color: SystemController.batteryPercent < 20 ? "#D92D20" : "#1C1C1E"
                    font.pixelSize: 12
                    font.family: "Inter, -apple-system, system-ui, sans-serif"
                }
            }

            Rectangle {
                width: 1
                height: 16
                color: "#E5E7EE"
            }

            // Power Session Trigger
            Rectangle {
                width: 26
                height: 26
                radius: 13
                color: pwrMouse.containsMouse ? "#E6EBFF" : "#F4F6FB"
                border.color: pwrMouse.containsMouse ? "#1428A0" : "#E5E7EE"
                border.width: 1

                Behavior on color { ColorAnimation { duration: 180; easing.type: Easing.OutCubic } }

                Text {
                    anchors.centerIn: parent
                    text: "⏻"
                    color: pwrMouse.containsMouse ? "#1428A0" : "#1C1C1E"
                    font.pixelSize: 12
                }

                MouseArea {
                    id: pwrMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: powerMenu.open()
                }

                Menu {
                    id: powerMenu
                    y: parent.height + 6

                    MenuItem {
                        text: "Lock Screen"
                        onTriggered: AuthManager.lockSession()
                    }
                    MenuItem {
                        text: "Sleep / Suspend"
                        onTriggered: SystemController.suspend()
                    }
                    MenuItem {
                        text: "Restart Machine"
                        onTriggered: SystemController.reboot()
                    }
                    MenuItem {
                        text: "Shut Down"
                        onTriggered: SystemController.powerOff()
                    }
                }
            }
        }
    }
}
