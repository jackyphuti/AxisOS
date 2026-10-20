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

    // Spawn centered with subtle 200ms ease-out scale
    x: (parent.width - width) * 0.5 + Math.floor(Math.random() * 30) - 15
    y: (parent.height - height) * 0.5 + Math.floor(Math.random() * 20) - 10

    scale: 1.0
    Behavior on scale {
        NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
    }

    Behavior on x {
        enabled: !dragArea.drag.active
        NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
    }
    Behavior on y {
        enabled: !dragArea.drag.active
        NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
    }

    // -------------------------------------------------------------------------
    // One UI Window Frame: Large Rounded Corners & Soft Drop Shadow
    // -------------------------------------------------------------------------
    Rectangle {
        id: windowFrame
        anchors.fill: parent
        radius: 24
        color: "#FFFFFF"
        border.color: dragArea.drag.active ? "#1428A0" : "#E5E7EE"
        border.width: 1

        Behavior on border.color {
            ColorAnimation { duration: 180; easing.type: Easing.OutCubic }
        }

        // Soft elevation shadow (no glowing neon rim)
        Rectangle {
            anchors.fill: parent
            anchors.topMargin: 4
            z: -1
            radius: 24
            color: "transparent"
            border.color: "#0D1428A0"
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
            radius: 24
            color: "#FFFFFF"

            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: "#E5E7EE"
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
                        color: "#2BB673"
                        font.pixelSize: 11
                        font.bold: true
                        font.family: "Inter, -apple-system, system-ui, sans-serif"
                    }

                    // Feature 2A: Time-Travel Checkpoint Button
                    Rectangle {
                        width: 26; height: 24; radius: 12
                        color: cpMouse.containsMouse ? "#E6EBFF" : "#F4F6FB"
                        border.color: cpMouse.containsMouse ? "#1428A0" : "#E5E7EE"; border.width: 1
                        Text { anchors.centerIn: parent; text: "⏱"; font.pixelSize: 11; color: "#1428A0" }
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

                    // Feature 4: Privacy Shield Toggle
                    Rectangle {
                        width: 26; height: 24; radius: 12
                        color: windowRoot.privacyShieldEnabled ? "#1428A0" : (psMouse.containsMouse ? "#E6EBFF" : "#F4F6FB")
                        border.color: windowRoot.privacyShieldEnabled ? "#1428A0" : "#E5E7EE"; border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: "🛡"
                            font.pixelSize: 11
                            color: windowRoot.privacyShieldEnabled ? "#FFFFFF" : "#6B6F7B"
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
                        color: (windowRoot.parent && windowRoot.parent.pendingNodeSource === windowRoot) ? "#2BB673" : (npMouse.containsMouse ? "#E6EBFF" : "#F4F6FB")
                        border.color: (windowRoot.parent && windowRoot.parent.pendingNodeSource === windowRoot) ? "#2BB673" : "#E5E7EE"
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: "◉"
                            font.pixelSize: 11
                            color: (windowRoot.parent && windowRoot.parent.pendingNodeSource === windowRoot) ? "#FFFFFF" : "#1428A0"
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
                    color: "#1C1C1E"
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
                        color: btnMinMouse.containsMouse ? "#F4F6FB" : "transparent"
                        Text { anchors.centerIn: parent; text: "−"; font.pixelSize: 14; font.bold: true; color: "#6B6F7B" }
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
                        color: btnMaxMouse.containsMouse ? "#F4F6FB" : "transparent"
                        Text { anchors.centerIn: parent; text: "◻"; font.pixelSize: 11; font.bold: true; color: "#6B6F7B" }
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
                        color: btnCloseMouse.containsMouse ? "#FEE2E2" : "transparent"
                        Text { anchors.centerIn: parent; text: "✕"; font.pixelSize: 11; font.bold: true; color: btnCloseMouse.containsMouse ? "#DC2626" : "#6B6F7B" }
                        MouseArea {
                            id: btnCloseMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (toplevel) toplevel.sendClose();
                            }
                        }
                    }
                }
            }

            // Drag Area for Titlebar
            MouseArea {
                id: dragArea
                anchors.fill: parent
                drag.target: windowRoot
                drag.axis: Drag.XAndYAxis
                drag.minimumX: -windowRoot.width + 120
                drag.maximumX: (windowRoot.parent ? windowRoot.parent.width : 1920) - 120
                drag.minimumY: 38
                drag.maximumY: (windowRoot.parent ? windowRoot.parent.height : 1080) - 60

                onPressed: {
                    windowRoot.z = ++windowRoot.parent.topZ;
                    windowRoot.isFocusedWindow = true;
                    var t = (windowRoot.toplevel && windowRoot.toplevel.title) ? windowRoot.toplevel.title : "Application";
                    SystemController.boostFocusedWindow(t, false);
                }
            }
        }

        // ---------------------------------------------------------------------
        // Grouped Content Area on --surface-alt (#F4F6FB)
        // ---------------------------------------------------------------------
        Rectangle {
            id: contentContainer
            anchors.top: titleBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 4
            radius: 20
            color: "#F4F6FB"

            // Client Surface Container
            ShellSurfaceItem {
                id: shellItem
                anchors.centerIn: parent
                shellSurface: windowRoot.shellSurface
                opacity: (windowRoot.privacyShieldEnabled && (AuthManager.shoulderSurfingDetected || !windowRoot.isFocusedWindow)) ? 0.05 : 1.0
                onSurfaceDestroyed: windowRoot.destroy()
            }

            // Biometric Privacy Veil
            Rectangle {
                id: privacyVeil
                anchors.fill: parent
                radius: 20
                color: "#EBF0FA"
                border.color: "#1428A0"
                border.width: 1
                visible: windowRoot.privacyShieldEnabled && (AuthManager.shoulderSurfingDetected || !windowRoot.isFocusedWindow)

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 8
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: "🛡 Privacy Shield Active"
                        color: "#1428A0"
                        font.bold: true
                        font.pixelSize: 14
                        font.family: "Inter, -apple-system, system-ui, sans-serif"
                    }
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: "Window content veiled to prevent shoulder-surfing."
                        color: "#6B6F7B"
                        font.pixelSize: 12
                        font.family: "Inter, -apple-system, system-ui, sans-serif"
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        windowRoot.isFocusedWindow = true;
                        AuthManager.shoulderSurfingDetected = false;
                    }
                }
            }
        }
    }
}
