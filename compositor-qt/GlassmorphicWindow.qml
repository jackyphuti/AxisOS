import QtQuick
import QtQuick.Layouts
import QtWayland.Compositor

Item {
    id: windowRoot
    property var shellSurface: null
    property var toplevel: null
    property bool privacyShieldEnabled: false
    property bool isFocusedWindow: true
    property string rewindStatusText: "" 
    property bool isMaximized: false
    property real savedX: 0
    property real savedY: 0

    width: shellItem.width + 16
    height: shellItem.height + 56

    // Spawn centered with smooth fluid spring scale
    x: (parent.width - width) * 0.5 + Math.floor(Math.random() * 30) - 15
    y: (parent.height - height) * 0.5 + Math.floor(Math.random() * 20) - 10

    scale: 1.0
    Behavior on scale {
        SpringAnimation { spring: Theme.springConstant; damping: Theme.springDamping; epsilon: Theme.springEpsilon }
    }

    Behavior on x {
        enabled: !dragArea.drag.active
        SmoothedAnimation { velocity: 1200 }
    }
    Behavior on y {
        enabled: !dragArea.drag.active
        SmoothedAnimation { velocity: 1200 }
    }

    // -------------------------------------------------------------------------
    // One UI Window Frame: 24px Rounded Corners & Soft Drop Shadow
    // -------------------------------------------------------------------------
    Rectangle {
        id: windowFrame
        anchors.fill: parent
        radius: Theme.radiusLg
        color: Theme.surface
        border.color: dragArea.drag.active ? Theme.accent : Theme.divider
        border.width: 1

        Behavior on border.color {
            ColorAnimation { duration: 180; easing.type: Easing.OutCubic }
        }

        // 1px Inner specular highlight border for contrast & legibility guards
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: Theme.radiusLg - 1
            color: "transparent"
            border.color: Theme.specularBorder
            border.width: 1
            z: 10
        }

        // Soft elevation shadow (no glowing neon rim)
        Rectangle {
            anchors.fill: parent
            anchors.topMargin: 4
            z: -1
            radius: Theme.radiusLg
            color: "transparent"
            border.color: Theme.shadowColor
            border.width: 8
            opacity: 0.6
        }

        // ---------------------------------------------------------------------
        // Titlebar (Clean, centered title, plain subtle icon buttons)
        // ---------------------------------------------------------------------
        Rectangle {
            id: titleBar
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 42
            radius: Theme.radiusLg
            color: Theme.surface

            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: Theme.divider
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 12

                // Left: AxisOS Signature Actions (Subtle Soft Pills)
                RowLayout {
                    spacing: 6

                    // Toast status
                    Text {
                        visible: windowRoot.rewindStatusText.length > 0
                        text: windowRoot.rewindStatusText
                        color: Theme.success
                        font.pixelSize: 11
                        font.bold: true
                        font.family: "Inter, -apple-system, system-ui, sans-serif"
                    }

                    // Feature 2A: Time-Travel Checkpoint Button (Clean Vector Clock/Snap)
                    Rectangle {
                        width: 26; height: 24; radius: 12
                        color: cpMouse.containsMouse ? Theme.accentSoft : Theme.surfaceAlt
                        border.color: cpMouse.containsMouse ? Theme.accent : Theme.divider
                        border.width: 1
                        Text { anchors.centerIn: parent; text: "◷"; font.pixelSize: 13; color: Theme.accent }
                        MouseArea {
                            id: cpMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                var t = (windowRoot.toplevel && windowRoot.toplevel.title) ? windowRoot.toplevel.title : "window";
                                SystemController.checkpointWindow(t);
                                windowRoot.rewindStatusText = "✓ Saved";
                            }
                        }
                    }

                    // Feature 4: Privacy Shield Toggle (Clean Vector Shield/Lock Glyph)
                    Rectangle {
                        width: 26; height: 24; radius: 12
                        color: windowRoot.privacyShieldEnabled ? Theme.accent : (psMouse.containsMouse ? Theme.accentSoft : Theme.surfaceAlt)
                        border.color: windowRoot.privacyShieldEnabled ? Theme.accent : Theme.divider
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: "⛉"
                            font.pixelSize: 12
                            color: windowRoot.privacyShieldEnabled ? "#FFFFFF" : Theme.textSecondary
                        }
                        MouseArea {
                            id: psMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: windowRoot.privacyShieldEnabled = !windowRoot.privacyShieldEnabled
                        }
                    }

                    // Feature 1: LiquidNode Piping Port
                    Rectangle {
                        width: 26; height: 24; radius: 12
                        color: (windowRoot.parent && windowRoot.parent.pendingNodeSource === windowRoot) ? Theme.success : (npMouse.containsMouse ? Theme.accentSoft : Theme.surfaceAlt)
                        border.color: (windowRoot.parent && windowRoot.parent.pendingNodeSource === windowRoot) ? Theme.success : Theme.divider
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: "◉"
                            font.pixelSize: 11
                            color: (windowRoot.parent && windowRoot.parent.pendingNodeSource === windowRoot) ? "#FFFFFF" : Theme.accent
                        }
                        MouseArea {
                            id: npMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (windowRoot.parent && windowRoot.parent.handleNodePortClicked) {
                                    windowRoot.parent.handleNodePortClicked(windowRoot);
                                }
                            }
                        }
                    }
                }

                // Center: Plain Centered Title
                Text {
                    Layout.fillWidth: true
                    text: toplevel && toplevel.title ? toplevel.title : "Application Window"
                    color: Theme.textPrimary
                    font.bold: true
                    font.pixelSize: 13
                    font.family: "Inter, -apple-system, system-ui, sans-serif"
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                }

                // Right: Plain subtle window control icon buttons (No colored traffic dots)
                Row {
                    spacing: 4

                    // Minimize Button
                    Rectangle {
                        width: 26; height: 26; radius: 13
                        color: btnMinMouse.containsMouse ? Theme.surfaceAlt : "transparent"
                        Text { anchors.centerIn: parent; text: "−"; font.pixelSize: 14; font.bold: true; color: Theme.textSecondary }
                        MouseArea {
                            id: btnMinMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: windowRoot.visible = false
                        }
                    }

                    // Maximize / Restore Button
                    Rectangle {
                        width: 26; height: 26; radius: 13
                        color: btnMaxMouse.containsMouse ? Theme.surfaceAlt : "transparent"
                        Text { anchors.centerIn: parent; text: "◻"; font.pixelSize: 11; font.bold: true; color: Theme.textSecondary }
                        MouseArea {
                            id: btnMaxMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (!windowRoot.isMaximized) {
                                    windowRoot.savedX = windowRoot.x;
                                    windowRoot.savedY = windowRoot.y;
                                    windowRoot.isMaximized = true;
                                    windowRoot.x = 8;
                                    windowRoot.y = 44;
                                    if (windowRoot.parent) {
                                        windowRoot.width = windowRoot.parent.width - 16;
                                        windowRoot.height = windowRoot.parent.height - 52 - 68;
                                    }
                                } else {
                                    windowRoot.isMaximized = false;
                                    windowRoot.x = windowRoot.savedX;
                                    windowRoot.y = windowRoot.savedY;
                                    windowRoot.width = shellItem.width + 16;
                                    windowRoot.height = shellItem.height + 56;
                                }
                            }
                        }
                    }

                    // Close Button
                    Rectangle {
                        width: 26; height: 26; radius: 13
                        color: btnCloseMouse.containsMouse ? (Theme.isDark ? "#4C1D24" : "#FEE2E2") : "transparent"
                        Text { anchors.centerIn: parent; text: "✕"; font.pixelSize: 11; font.bold: true; color: btnCloseMouse.containsMouse ? Theme.danger : Theme.textSecondary }
                        MouseArea {
                            id: btnCloseMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (windowRoot.shellSurface) {
                                    windowRoot.shellSurface.sendClose();
                                } else {
                                    windowRoot.destroy();
                                }
                            }
                        }
                    }
                }
            }

            // Window Dragging Area
            MouseArea {
                id: dragArea
                anchors.fill: parent
                drag.target: windowRoot
                drag.axis: Drag.XAndYAxis
                z: -1

                onPressed: {
                    if (windowRoot.parent && windowRoot.parent.topZ !== undefined) {
                        windowRoot.z = ++windowRoot.parent.topZ;
                    }
                }

                onDoubleClicked: {
                    if (!windowRoot.isMaximized) {
                        windowRoot.savedX = windowRoot.x;
                        windowRoot.savedY = windowRoot.y;
                        windowRoot.isMaximized = true;
                        windowRoot.x = 8;
                        windowRoot.y = 44;
                        if (windowRoot.parent) {
                            windowRoot.width = windowRoot.parent.width - 16;
                            windowRoot.height = windowRoot.parent.height - 52 - 68;
                        }
                    } else {
                        windowRoot.isMaximized = false;
                        windowRoot.x = windowRoot.savedX;
                        windowRoot.y = windowRoot.savedY;
                        windowRoot.width = shellItem.width + 16;
                        windowRoot.height = shellItem.height + 56;
                    }
                }
            }
        }

        // ---------------------------------------------------------------------
        // Client Wayland Surface Area
        // ---------------------------------------------------------------------
        Item {
            id: contentContainer
            anchors.top: titleBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 8
            clip: true

            ShellSurfaceItem {
                id: shellItem
                shellSurface: windowRoot.shellSurface
                autoCreatePopupItems: true
                anchors.centerIn: parent

                onSurfaceDestroyed: {
                    windowRoot.destroy();
                }
            }

            // Privacy Shield Glassmorphic Overlay
            Rectangle {
                anchors.fill: parent
                visible: windowRoot.privacyShieldEnabled
                color: Theme.isDark ? "#F21F2231" : "#F2FFFFFF"
                radius: Theme.radiusMd

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 8

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: "⛉"
                        font.pixelSize: 32
                        color: Theme.accent
                    }
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: "Privacy Shield Active"
                        font.bold: true
                        font.pixelSize: 14
                        color: Theme.textPrimary
                    }
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: "Window contents masked from screen capture"
                        font.pixelSize: 12
                        color: Theme.textSecondary
                    }
                }
            }
        }
    }
}
