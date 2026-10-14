import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: topBar
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right
    height: 36
    color: "#D90B0D13" // Deep obsidian frosted glass
    border.color: "#242A38" // Matte titanium rim
    border.width: 1

    // Cyber-Teal / Violet bottom highlight line
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "#00F2FE" }
            GradientStop { position: 0.5; color: "#8B5CF6" }
            GradientStop { position: 1.0; color: "#FF007F" }
        }
        opacity: 0.45
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 18

        // Left Branding & Kernel Telemetry
        RowLayout {
            spacing: 10

            // Glowing Liquid Node Logo
            Rectangle {
                width: 12
                height: 12
                radius: 6
                color: "#00F2FE"

                Rectangle {
                    anchors.centerIn: parent
                    width: 6
                    height: 6
                    radius: 3
                    color: "#FFFFFF"
                }
            }

            Text {
                text: "AXIS OS"
                color: "#F8FAFC"
                font.bold: true
                font.pixelSize: 12
                font.family: "Inter, Cantarell, sans-serif"
            }

            Text {
                text: "LiquidNodes"
                color: "#8B5CF6"
                font.bold: true
                font.pixelSize: 11
                font.family: "Inter, Cantarell, sans-serif"
            }

            // XanMod Kernel Badge
            Rectangle {
                height: 18
                width: kernelTxt.implicitWidth + 12
                radius: 4
                color: "#181C26"
                border.color: "#283042"
                border.width: 1

                Text {
                    id: kernelTxt
                    anchors.centerIn: parent
                    text: SystemController.kernelVersion
                    color: "#94A3B8"
                    font.pixelSize: 9
                    font.bold: true
                    font.family: "Fira Code, monospace"
                }
            }
        }

        Item { Layout.fillWidth: true }

        // Center Date & Time Clock
        Text {
            id: clockDisplay
            color: "#F8FAFC"
            font.bold: true
            font.pixelSize: 12
            font.family: "Inter, Cantarell, sans-serif"

            Timer {
                interval: 1000
                running: true
                repeat: true
                triggeredOnStart: true
                onTriggered: {
                    var now = new Date();
                    clockDisplay.text = Qt.formatDateTime(now, "ddd MMM d   hh:mm AP");
                }
            }
        }

        Item { Layout.fillWidth: true }

        // Right Telemetry & Hardware Indicators (D-Bus Bound)
        RowLayout {
            spacing: 14

            // CPU Load Meter
            RowLayout {
                spacing: 5
                Text {
                    text: "CPU"
                    color: "#64748B"
                    font.pixelSize: 10
                    font.bold: true
                }
                Text {
                    text: SystemController.cpuLoadPercent + "%"
                    color: SystemController.cpuLoadPercent > 80 ? "#FF007F" : "#00F2FE"
                    font.pixelSize: 11
                    font.bold: true
                    font.family: "Fira Code, monospace"
                }
            }

            // RAM Utilization
            RowLayout {
                spacing: 5
                Text {
                    text: "RAM"
                    color: "#64748B"
                    font.pixelSize: 10
                    font.bold: true
                }
                Text {
                    text: SystemController.ramUsagePercent + "%"
                    color: "#8B5CF6"
                    font.pixelSize: 11
                    font.bold: true
                    font.family: "Fira Code, monospace"
                }
            }

            // zRAM Compression Savings
            RowLayout {
                spacing: 5
                Text {
                    text: "zRAM"
                    color: "#64748B"
                    font.pixelSize: 10
                    font.bold: true
                }
                Text {
                    text: "+" + SystemController.zramSavedMb + " MB"
                    color: "#00F2FE"
                    font.pixelSize: 11
                    font.bold: true
                    font.family: "Fira Code, monospace"
                }
            }

            Rectangle {
                width: 1
                height: 16
                color: "#283042"
            }

            // Wi-Fi Status
            RowLayout {
                spacing: 5
                Rectangle {
                    width: 7
                    height: 7
                    radius: 3.5
                    color: SystemController.wifiConnected ? "#00F2FE" : "#EF4444"
                }
                Text {
                    text: SystemController.wifiSsid
                    color: "#E2E8F0"
                    font.pixelSize: 11
                    font.bold: true
                }
            }

            // Battery Status
            RowLayout {
                spacing: 5
                Text {
                    text: (SystemController.batteryCharging ? "⚡ " : "") + SystemController.batteryPercent + "%"
                    color: SystemController.batteryCharging ? "#00F2FE" : (SystemController.batteryPercent < 20 ? "#EF4444" : "#E2E8F0")
                    font.pixelSize: 11
                    font.bold: true
                    font.family: "Fira Code, monospace"
                }
            }

            // Power Session Menu Trigger
            Rectangle {
                width: 24
                height: 24
                radius: 12
                color: pwrMouse.containsMouse ? "#FF007F" : "#181C26"
                border.color: "#283042"
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "⏻"
                    color: pwrMouse.containsMouse ? "#FFFFFF" : "#E2E8F0"
                    font.pixelSize: 12
                }

                MouseArea {
                    id: pwrMouse
                    anchors.fill: parent
                    hoverEnabled: true
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
