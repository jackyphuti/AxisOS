import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: quickSettingsRoot
    width: 360
    height: 480
    visible: false

    // Animate position and opacity on show/hide
    opacity: visible ? 1.0 : 0.0
    scale: visible ? 1.0 : 0.95
    y: visible ? 46 : 38

    Behavior on opacity {
        NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
    }
    Behavior on scale {
        SpringAnimation { spring: Theme.springConstant; damping: Theme.springDamping; epsilon: Theme.springEpsilon }
    }
    Behavior on y {
        SpringAnimation { spring: Theme.springConstant; damping: Theme.springDamping; epsilon: Theme.springEpsilon }
    }

    // Dismiss when clicking outside
    MouseArea {
        anchors.fill: parent
        onClicked: {} // Catch clicks inside panel
    }

    // Outer shadow
    Rectangle {
        anchors.fill: panelCard
        anchors.topMargin: 6
        radius: Theme.radiusLg
        color: "transparent"
        border.color: Theme.shadowColor
        border.width: 10
        opacity: 0.8
        z: -1
    }

    // Main Card Surface
    Rectangle {
        id: panelCard
        anchors.fill: parent
        radius: Theme.radiusLg
        color: Theme.surface
        border.color: Theme.divider
        border.width: 1
        clip: true

        // 1px Inner Specular Border
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: Theme.radiusLg - 1
            color: "transparent"
            border.color: Theme.specularBorder
            border.width: 1
            z: 10
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 14

            // 1. User Header & System Actions
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                // User Avatar Circle
                Rectangle {
                    width: 36
                    height: 36
                    radius: 18
                    color: Theme.accentSoft

                    Text {
                        anchors.centerIn: parent
                        text: "A"
                        color: Theme.accent
                        font.bold: true
                        font.pixelSize: 14
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 1

                    Text {
                        text: "axis"
                        color: Theme.textPrimary
                        font.bold: true
                        font.pixelSize: 15
                    }
                    Text {
                        text: "AxisOS 2.1"
                        color: Theme.textSecondary
                        font.pixelSize: 11
                    }
                }

                // Header Action Buttons (Settings, Lock, Power)
                RowLayout {
                    spacing: 6

                    // Settings Cog
                    Rectangle {
                        width: 32
                        height: 32
                        radius: Theme.radiusPill
                        color: sMouse.containsMouse ? Theme.accentSoft : Theme.surfaceAlt

                        Text {
                            anchors.centerIn: parent
                            text: "⚙"
                            color: Theme.textPrimary
                            font.pixelSize: 14
                        }
                        MouseArea {
                            id: sMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                quickSettingsRoot.visible = false;
                                SystemController.launchApp("axis-settings");
                            }
                        }
                    }

                    // Lock Button
                    Rectangle {
                        width: 32
                        height: 32
                        radius: Theme.radiusPill
                        color: lMouse.containsMouse ? Theme.accentSoft : Theme.surfaceAlt

                        Text {
                            anchors.centerIn: parent
                            text: "🔒"
                            color: Theme.textPrimary
                            font.pixelSize: 13
                        }
                        MouseArea {
                            id: lMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                quickSettingsRoot.visible = false;
                                AuthManager.lockSession();
                            }
                        }
                    }
                }
            }

            // Separator
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.divider
            }

            // 2. Quick Action Tiles (2 x 3 Grid)
            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 10
                rowSpacing: 10

                // Tile 1: Wi-Fi
                Rectangle {
                    Layout.fillWidth: true
                    height: 54
                    radius: Theme.radiusMd
                    color: SystemController.wifiConnected ? Theme.accent : Theme.surfaceAlt
                    border.color: Theme.divider
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8

                        Text {
                            text: "📶"
                            font.pixelSize: 15
                            color: SystemController.wifiConnected ? "#FFFFFF" : Theme.textPrimary
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1
                            Text {
                                text: "Wi-Fi"
                                font.bold: true
                                font.pixelSize: 12
                                color: SystemController.wifiConnected ? "#FFFFFF" : Theme.textPrimary
                            }
                            Text {
                                text: SystemController.wifiSsid
                                font.pixelSize: 10
                                elide: Text.ElideRight
                                Layout.maximumWidth: 100
                                color: SystemController.wifiConnected ? "#E6EBFF" : Theme.textSecondary
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: SystemController.toggleWifi(!SystemController.wifiConnected)
                    }
                }

                // Tile 2: Bluetooth
                Rectangle {
                    Layout.fillWidth: true
                    height: 54
                    radius: Theme.radiusMd
                    color: SystemController.bluetoothEnabled ? Theme.accent : Theme.surfaceAlt
                    border.color: Theme.divider
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8

                        Text {
                            text: "ᛒ"
                            font.bold: true
                            font.pixelSize: 15
                            color: SystemController.bluetoothEnabled ? "#FFFFFF" : Theme.textPrimary
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1
                            Text {
                                text: "Bluetooth"
                                font.bold: true
                                font.pixelSize: 12
                                color: SystemController.bluetoothEnabled ? "#FFFFFF" : Theme.textPrimary
                            }
                            Text {
                                text: SystemController.bluetoothEnabled ? (SystemController.bluetoothConnectedDevices + " Connected") : "Off"
                                font.pixelSize: 10
                                color: SystemController.bluetoothEnabled ? "#E6EBFF" : Theme.textSecondary
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: SystemController.toggleBluetooth(!SystemController.bluetoothEnabled)
                    }
                }

                // Tile 3: Dark Mode (Live One UI Synchronizer)
                Rectangle {
                    Layout.fillWidth: true
                    height: 54
                    radius: Theme.radiusMd
                    color: Theme.isDark ? Theme.accent : Theme.surfaceAlt
                    border.color: Theme.divider
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8

                        Text {
                            text: Theme.isDark ? "🌙" : "☀️"
                            font.pixelSize: 15
                            color: Theme.isDark ? "#FFFFFF" : Theme.textPrimary
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1
                            Text {
                                text: "Dark Mode"
                                font.bold: true
                                font.pixelSize: 12
                                color: Theme.isDark ? "#FFFFFF" : Theme.textPrimary
                            }
                            Text {
                                text: Theme.isDark ? "Obsidian" : "Airy Light"
                                font.pixelSize: 10
                                color: Theme.isDark ? "#E6EBFF" : Theme.textSecondary
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: SystemController.toggleDarkMode()
                    }
                }

                // Tile 4: GameBoost / Performance
                property bool gameBoostActive: false
                Rectangle {
                    Layout.fillWidth: true
                    height: 54
                    radius: Theme.radiusMd
                    color: parent.gameBoostActive ? Theme.accent : Theme.surfaceAlt
                    border.color: Theme.divider
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8

                        Text {
                            text: "⚡"
                            font.pixelSize: 15
                            color: parent.parent.gameBoostActive ? "#FFFFFF" : Theme.textPrimary
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1
                            Text {
                                text: "GameBoost"
                                font.bold: true
                                font.pixelSize: 12
                                color: parent.parent.parent.gameBoostActive ? "#FFFFFF" : Theme.textPrimary
                            }
                            Text {
                                text: parent.parent.parent.gameBoostActive ? "Performance" : "Schedutil"
                                font.pixelSize: 10
                                color: parent.parent.parent.gameBoostActive ? "#E6EBFF" : Theme.textSecondary
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            parent.parent.gameBoostActive = !parent.parent.gameBoostActive;
                            SystemController.boostFocusedWindow("GameBoost", parent.parent.gameBoostActive);
                        }
                    }
                }

                // Tile 5: Do Not Disturb
                property bool dndActive: false
                Rectangle {
                    Layout.fillWidth: true
                    height: 54
                    radius: Theme.radiusMd
                    color: parent.dndActive ? Theme.accent : Theme.surfaceAlt
                    border.color: Theme.divider
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8

                        Text {
                            text: "🔕"
                            font.pixelSize: 15
                            color: parent.parent.dndActive ? "#FFFFFF" : Theme.textPrimary
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1
                            Text {
                                text: "Do Not Disturb"
                                font.bold: true
                                font.pixelSize: 12
                                color: parent.parent.parent.dndActive ? "#FFFFFF" : Theme.textPrimary
                            }
                            Text {
                                text: parent.parent.parent.dndActive ? "Muted" : "Alerts On"
                                font.pixelSize: 10
                                color: parent.parent.parent.dndActive ? "#E6EBFF" : Theme.textSecondary
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: parent.parent.dndActive = !parent.parent.dndActive
                    }
                }

                // Tile 6: Night Light
                property bool nightLightActive: false
                Rectangle {
                    Layout.fillWidth: true
                    height: 54
                    radius: Theme.radiusMd
                    color: parent.nightLightActive ? Theme.accent : Theme.surfaceAlt
                    border.color: Theme.divider
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8

                        Text {
                            text: "👁"
                            font.pixelSize: 15
                            color: parent.parent.nightLightActive ? "#FFFFFF" : Theme.textPrimary
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1
                            Text {
                                text: "Night Light"
                                font.bold: true
                                font.pixelSize: 12
                                color: parent.parent.parent.nightLightActive ? "#FFFFFF" : Theme.textPrimary
                            }
                            Text {
                                text: parent.parent.parent.nightLightActive ? "4500K Warm" : "Standard"
                                font.pixelSize: 10
                                color: parent.parent.parent.nightLightActive ? "#E6EBFF" : Theme.textSecondary
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: parent.parent.nightLightActive = !parent.parent.nightLightActive
                    }
                }
            }

            // 3. Hardware Brightness Slider
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Text {
                    text: "☀️"
                    color: Theme.textSecondary
                    font.pixelSize: 14
                }

                Slider {
                    id: brightSlider
                    Layout.fillWidth: true
                    from: 10
                    to: 100
                    value: SystemController.brightness
                    onMoved: SystemController.setBrightness(Math.round(value))
                }

                Text {
                    text: Math.round(brightSlider.value) + "%"
                    color: Theme.textSecondary
                    font.pixelSize: 11
                    Layout.preferredWidth: 32
                }
            }

            // 4. PipeWire Audio Volume Slider
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Text {
                    text: SystemController.isMuted ? "🔇" : "🔊"
                    color: Theme.textSecondary
                    font.pixelSize: 14

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: SystemController.setMuted(!SystemController.isMuted)
                    }
                }

                Slider {
                    id: volSlider
                    Layout.fillWidth: true
                    from: 0
                    to: 100
                    value: SystemController.volume
                    onMoved: SystemController.setVolume(Math.round(value))
                }

                Text {
                    text: SystemController.isMuted ? "Mute" : (Math.round(volSlider.value) + "%")
                    color: Theme.textSecondary
                    font.pixelSize: 11
                    Layout.preferredWidth: 32
                }
            }

            Item { Layout.fillHeight: true }

            // 5. Footer Quick Link
            Rectangle {
                Layout.fillWidth: true
                height: 36
                radius: Theme.radiusPill
                color: fMouse.containsMouse ? Theme.accentSoft : Theme.surfaceAlt

                Text {
                    anchors.centerIn: parent
                    text: "All Settings & Hardware →"
                    color: Theme.accent
                    font.bold: true
                    font.pixelSize: 12
                }

                MouseArea {
                    id: fMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        quickSettingsRoot.visible = false;
                        SystemController.launchApp("axis-settings");
                    }
                }
            }
        }
    }
}
