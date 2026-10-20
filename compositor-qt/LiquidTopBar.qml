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

        // Left Branding & Launcher Trigger (Clean, Borderless Flat Text + Accent Dot)
        RowLayout {
            spacing: 10

            Rectangle {
                id: brandBtn
                height: 28
                width: brandRow.implicitWidth + 12
                radius: 8
                color: brandMouse.containsMouse ? "#F4F6FB" : "transparent"
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
                        color: "#1428A0"
                    }

                    Text {
                        text: "AxisOS"
                        color: "#1C1C1E"
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

            // Wi-Fi Status (Clean Vector Icon + Live Active SSID)
            RowLayout {
                spacing: 6

                // Vector Wi-Fi Signal Icon (3 Arcs Canvas)
                Canvas {
                    id: wifiCanvas
                    width: 16
                    height: 14
                    renderTarget: Canvas.FramebufferObject
                    onPaint: {
                        var ctx = getContext("2d");
                        ctx.reset();
                        ctx.lineWidth = 1.75;
                        ctx.lineCap = "round";
                        var active = SystemController.wifiConnected;
                        ctx.strokeStyle = active ? "#1428A0" : "#6B6F7B";
                        ctx.fillStyle = active ? "#1428A0" : "#6B6F7B";

                        // Base dot
                        ctx.beginPath();
                        ctx.arc(8, 12, 1.4, 0, 2 * Math.PI);
                        ctx.fill();

                        // Inner arc
                        ctx.beginPath();
                        ctx.arc(8, 12, 4.5, -0.75 * Math.PI, -0.25 * Math.PI);
                        ctx.stroke();

                        // Outer arc
                        ctx.beginPath();
                        ctx.arc(8, 12, 8.5, -0.75 * Math.PI, -0.25 * Math.PI);
                        ctx.stroke();
                    }

                    Connections {
                        target: SystemController
                        function onWifiChanged() { wifiCanvas.requestPaint(); }
                    }
                }

                Text {
                    text: SystemController.wifiSsid
                    color: "#1C1C1E"
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    font.family: "Inter, -apple-system, system-ui, sans-serif"
                }
            }

            // Battery / Power Status (Clean Vector Battery Pill)
            RowLayout {
                spacing: 6
                visible: SystemController.hasBattery || SystemController.batteryPercent > 0

                // Vector Battery Pill
                Item {
                    width: 24
                    height: 12

                    // Battery Body Outline
                    Rectangle {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        width: 20
                        height: 11
                        radius: 2.5
                        color: "transparent"
                        border.color: (SystemController.batteryPercent < 20 && !SystemController.batteryCharging) ? "#D92D20" : "#1C1C1E"
                        border.width: 1

                        // Battery Capacity Fill
                        Rectangle {
                            anchors.left: parent.left
                            anchors.leftMargin: 1.5
                            anchors.verticalCenter: parent.verticalCenter
                            height: 7
                            width: Math.max(2, Math.min(16, (SystemController.batteryPercent / 100.0) * 16))
                            radius: 1.5
                            color: SystemController.batteryCharging ? "#2BB673" :
                                   (SystemController.batteryPercent < 20 ? "#D92D20" : "#1428A0")
                        }
                    }

                    // Battery Terminal Cap
                    Rectangle {
                        anchors.right: parent.right
                        anchors.rightMargin: 1
                        anchors.verticalCenter: parent.verticalCenter
                        width: 2
                        height: 5
                        radius: 1
                        color: (SystemController.batteryPercent < 20 && !SystemController.batteryCharging) ? "#D92D20" : "#1C1C1E"
                    }
                }

                Text {
                    text: (SystemController.batteryCharging ? "Charging " : "") + SystemController.batteryPercent + "%"
                    color: (SystemController.batteryPercent < 20 && !SystemController.batteryCharging) ? "#D92D20" : "#1C1C1E"
                    font.pixelSize: 12
                    font.weight: Font.Medium
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
