import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: topBar
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right
    height: 38
    color: Theme.surface
    border.color: Theme.divider
    border.width: 1

    signal quickSettingsRequested()

    // 1px specular border at bottom for legibility guard
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: Theme.specularBorder
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 18
        anchors.rightMargin: 18
        spacing: 16

        // Left Branding & Launcher Trigger (Clean, Borderless Flat Text + Accent Dot)
        RowLayout {
            spacing: 10

            Rectangle {
                id: brandBtn
                height: 28
                width: brandRow.implicitWidth + 12
                radius: Theme.radiusSm
                color: brandMouse.containsMouse ? Theme.surfaceAlt : "transparent"
                border.width: 0

                Behavior on color { ColorAnimation { duration: 150; easing.type: Easing.OutCubic } }

                RowLayout {
                    id: brandRow
                    anchors.centerIn: parent
                    spacing: 7

                    // Clean Samsung Deep Blue Accent Dot
                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: Theme.accent
                    }

                    Text {
                        text: "AxisOS"
                        color: Theme.textPrimary
                        font.bold: true
                        font.pixelSize: 13
                        font.letterSpacing: -0.2
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

            Rectangle {
                width: 1
                height: 16
                color: Theme.divider
            }

            // Virtual Workspace Indicator Pills (Clean, Borderless Pills)
            RowLayout {
                spacing: 4

                Repeater {
                    model: ["1", "2", "3"]
                    Rectangle {
                        width: index === 0 ? 22 : 18
                        height: 18
                        radius: Theme.radiusPill
                        color: index === 0 ? Theme.accent : Theme.surfaceAlt

                        Text {
                            anchors.centerIn: parent
                            text: modelData
                            color: index === 0 ? "#FFFFFF" : Theme.textSecondary
                            font.bold: index === 0
                            font.pixelSize: 10
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: console.log("[LiquidNodesOS] Switch to Workspace " + modelData)
                        }
                    }
                }
            }
        }

        Item { Layout.fillWidth: true }

        // Center Time & Date
        Text {
            id: clockText
            color: Theme.textPrimary
            font.pixelSize: 13
            font.weight: Font.SemiBold
            font.letterSpacing: -0.1
            font.family: "Inter, -apple-system, system-ui, sans-serif"

            function updateTime() {
                var d = new Date();
                var days = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"];
                var months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];
                var h = d.getHours().toString().padStart(2, '0');
                var m = d.getMinutes().toString().padStart(2, '0');
                clockText.text = days[d.getDay()] + " " + months[d.getMonth()] + " " + d.getDate() + "  " + h + ":" + m;
            }

            Component.onCompleted: updateTime()
        }

        Timer {
            interval: 1000
            running: true
            repeat: true
            onTriggered: clockText.updateTime()
        }

        Item { Layout.fillWidth: true }

        // Right Telemetry & Status Icons (Quick Settings Trigger Group)
        RowLayout {
            spacing: 12

            // Interactive Quick Settings Trigger Capsule
            Rectangle {
                height: 28
                width: statusRow.implicitWidth + 14
                radius: Theme.radiusPill
                color: qsMouse.containsMouse ? Theme.surfaceAlt : "transparent"
                border.color: qsMouse.containsMouse ? Theme.divider : "transparent"
                border.width: 1

                Behavior on color { ColorAnimation { duration: 150 } }

                RowLayout {
                    id: statusRow
                    anchors.centerIn: parent
                    spacing: 10

                    // Audio Volume Indicator
                    Text {
                        text: SystemController.isMuted ? "🔇" : (SystemController.volume > 60 ? "🔊" : "🔉")
                        color: Theme.textPrimary
                        font.pixelSize: 12
                    }

                    // Network SSID / Status
                    RowLayout {
                        spacing: 5
                        Canvas {
                            id: wifiCanvas
                            width: 14
                            height: 14

                            onPaint: {
                                var ctx = getContext("2d");
                                ctx.reset();
                                var active = SystemController.wifiConnected;
                                ctx.lineWidth = 1.6;
                                ctx.lineCap = "round";
                                ctx.strokeStyle = active ? Theme.accent : Theme.textSecondary;
                                ctx.fillStyle = active ? Theme.accent : Theme.textSecondary;

                                ctx.beginPath();
                                ctx.arc(7, 11, 1.2, 0, 2 * Math.PI);
                                ctx.fill();

                                ctx.beginPath();
                                ctx.arc(7, 11, 4.0, -0.75 * Math.PI, -0.25 * Math.PI);
                                ctx.stroke();

                                ctx.beginPath();
                                ctx.arc(7, 11, 7.2, -0.75 * Math.PI, -0.25 * Math.PI);
                                ctx.stroke();
                            }

                            Connections {
                                target: SystemController
                                function onWifiChanged() { wifiCanvas.requestPaint(); }
                            }
                        }

                        Text {
                            text: SystemController.wifiSsid
                            color: Theme.textPrimary
                            font.pixelSize: 12
                            font.weight: Font.Medium
                            font.family: "Inter, -apple-system, system-ui, sans-serif"
                        }
                    }

                    // Battery Indicator Pill
                    RowLayout {
                        spacing: 5
                        visible: SystemController.hasBattery || SystemController.batteryPercent > 0

                        Item {
                            width: 22
                            height: 12

                            Rectangle {
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                width: 18
                                height: 11
                                radius: 2.5
                                color: "transparent"
                                border.color: (SystemController.batteryPercent < 20 && !SystemController.batteryCharging) ? Theme.danger : Theme.textPrimary
                                border.width: 1

                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 1.5
                                    anchors.verticalCenter: parent.verticalCenter
                                    height: 7
                                    width: Math.max(2, Math.min(14, (SystemController.batteryPercent / 100.0) * 14))
                                    radius: 1.5
                                    color: SystemController.batteryCharging ? Theme.success :
                                           (SystemController.batteryPercent < 20 ? Theme.danger : Theme.accent)
                                }
                            }

                            Rectangle {
                                anchors.right: parent.right
                                anchors.rightMargin: 1
                                anchors.verticalCenter: parent.verticalCenter
                                width: 2
                                height: 5
                                radius: 1
                                color: (SystemController.batteryPercent < 20 && !SystemController.batteryCharging) ? Theme.danger : Theme.textPrimary
                            }
                        }

                        Text {
                            text: (SystemController.batteryCharging ? "⚡ " : "") + SystemController.batteryPercent + "%"
                            color: (SystemController.batteryPercent < 20 && !SystemController.batteryCharging) ? Theme.danger : Theme.textPrimary
                            font.pixelSize: 11
                            font.weight: Font.Medium
                            font.family: "Inter, -apple-system, system-ui, sans-serif"
                        }
                    }

                    // Control Center Dropdown Arrow
                    Text {
                        text: "▾"
                        color: Theme.textSecondary
                        font.pixelSize: 11
                    }
                }

                MouseArea {
                    id: qsMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: topBar.quickSettingsRequested()
                }
            }

            Rectangle {
                width: 1
                height: 16
                color: Theme.divider
            }

            // Power Session Trigger
            Rectangle {
                width: 26
                height: 26
                radius: Theme.radiusPill
                color: pwrMouse.containsMouse ? Theme.accentSoft : Theme.surfaceAlt
                border.color: pwrMouse.containsMouse ? Theme.accent : Theme.divider
                border.width: 1

                Behavior on color { ColorAnimation { duration: 180; easing.type: Easing.OutCubic } }

                Text {
                    anchors.centerIn: parent
                    text: "⏻"
                    color: pwrMouse.containsMouse ? Theme.accent : Theme.textPrimary
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
