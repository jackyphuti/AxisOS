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
    height: shellItem.height + 48

    // Center spawn with slight random offset
    x: (parent.width - width) * 0.5 + Math.floor(Math.random() * 40) - 20
    y: (parent.height - height) * 0.5 + Math.floor(Math.random() * 30) - 15

    // -------------------------------------------------------------------------
    // Jelly Physics Engine: Organic Elastic Drag & Inertia Dynamics
    // -------------------------------------------------------------------------
    property real prevX: x
    property real prevY: y
    property real velX: 0.0
    property real velY: 0.0

    Timer {
        id: velocityTracker
        interval: 16
        running: dragArea.drag.active
        repeat: true
        onTriggered: {
            var dx = windowRoot.x - windowRoot.prevX;
            var dy = windowRoot.y - windowRoot.prevY;
            windowRoot.velX = windowRoot.velX * 0.7 + dx * 0.3;
            windowRoot.velY = windowRoot.velY * 0.7 + dy * 0.3;
            windowRoot.prevX = windowRoot.x;
            windowRoot.prevY = windowRoot.y;
        }
    }

    // Dynamic jelly stretch, squash, and tilt while in motion
    property real targetScaleX: dragArea.drag.active ? (1.0 + Math.min(0.14, Math.abs(velX) * 0.008) - Math.min(0.06, Math.abs(velY) * 0.004)) : 1.0
    property real targetScaleY: dragArea.drag.active ? (1.0 + Math.min(0.14, Math.abs(velY) * 0.008) - Math.min(0.06, Math.abs(velX) * 0.004)) : 1.0
    property real targetRotation: dragArea.drag.active ? Math.max(-6.0, Math.min(6.0, velX * 0.25)) : 0.0

    // Organic Spring Animation physics on translation
    Behavior on x {
        enabled: !dragArea.drag.active
        SpringAnimation {
            spring: 3.8
            damping: 0.30
            epsilon: 0.20
            mass: 1.15
        }
    }

    Behavior on y {
        enabled: !dragArea.drag.active
        SpringAnimation {
            spring: 3.8
            damping: 0.30
            epsilon: 0.20
            mass: 1.15
        }
    }

    // Organic Jelly Spring Animation on deformation and tilt
    transform: [
        Scale {
            origin.x: windowRoot.width * 0.5
            origin.y: windowRoot.height * 0.5
            xScale: windowRoot.targetScaleX
            yScale: windowRoot.targetScaleY

            Behavior on xScale {
                SpringAnimation {
                    spring: 4.2
                    damping: 0.28
                    mass: 0.95
                }
            }

            Behavior on yScale {
                SpringAnimation {
                    spring: 4.2
                    damping: 0.28
                    mass: 0.95
                }
            }
        },
        Rotation {
            origin.x: windowRoot.width * 0.5
            origin.y: windowRoot.height * 0.5
            angle: windowRoot.targetRotation

            Behavior on angle {
                SpringAnimation {
                    spring: 4.5
                    damping: 0.32
                }
            }
        }
    ]

    // -------------------------------------------------------------------------
    // Floating Borderless Glassmorphic Prism Container
    // -------------------------------------------------------------------------
    Rectangle {
        id: prismFrame
        anchors.fill: parent
        radius: 12
        color: "#E00A0D14" // Deep obsidian translucent base

        // Cyber-Teal to Electric-Violet Neon Prism Rim Highlight
        border.color: dragArea.drag.active ? "#00F2FE" : "#30374A"
        border.width: dragArea.drag.active ? 2 : 1

        // Top-edge light refraction highlight
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            radius: 12
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#00F2FE" }
                GradientStop { position: 0.5; color: "#8B5CF6" }
                GradientStop { position: 1.0; color: "#FF007F" }
            }
            opacity: dragArea.drag.active ? 0.95 : 0.60
        }

        // ---------------------------------------------------------------------
        // Frosted Glass Titlebar
        // ---------------------------------------------------------------------
        Rectangle {
            id: titleBar
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 34
            radius: 12
            color: "#CC121622"

            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: "#252B3B"
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 10

                // Window Controls
                Row {
                    spacing: 7

                    // Close Button
                    Rectangle {
                        width: 12; height: 12; radius: 6
                        color: btnCloseMouse.containsMouse ? "#EF4444" : "#DC2626"

                        MouseArea {
                            id: btnCloseMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                if (toplevel) toplevel.sendClose();
                            }
                        }
                    }

                    // Minimize Button
                    Rectangle {
                        width: 12; height: 12; radius: 6
                        color: btnMinMouse.containsMouse ? "#FBBF24" : "#F59E0B"
                        MouseArea {
                            id: btnMinMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                windowRoot.visible = false;
                            }
                        }
                    }

                    // Maximize / Restore Button
                    Rectangle {
                        width: 12; height: 12; radius: 6
                        color: btnMaxMouse.containsMouse ? "#34D399" : "#10B981"

                        MouseArea {
                            id: btnMaxMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                if (!windowRoot.isMaximized) {
                                    windowRoot.savedX = windowRoot.x;
                                    windowRoot.savedY = windowRoot.y;
                                    windowRoot.isMaximized = true;
                                    windowRoot.x = 8;
                                    windowRoot.y = 40;
                                    if (windowRoot.parent) {
                                        windowRoot.width = windowRoot.parent.width - 16;
                                        windowRoot.height = windowRoot.parent.height - 48 - 64;
                                    }
                                } else {
                                    windowRoot.isMaximized = false;
                                    windowRoot.x = windowRoot.savedX;
                                    windowRoot.y = windowRoot.savedY;
                                    windowRoot.width = shellItem.width + 16;
                                    windowRoot.height = shellItem.height + 48;
                                }
                            }
                        }
                    }
                }

                // Window Title Header
                Text {
                    Layout.fillWidth: true
                    text: toplevel && toplevel.title ? toplevel.title : "Application Window"
                    color: "#E2E8F0"
                    font.bold: true
                    font.pixelSize: 11
                    font.family: "Inter, Cantarell, sans-serif"
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                }

                // Signature AxisOS Window Action Pills (Rewind, Privacy Shield & LiquidNode Port)
                RowLayout {
                    spacing: 6
                    z: 20

                    // Status toast badge for Rewind / Node actions
                    Text {
                        visible: windowRoot.rewindStatusText.length > 0
                        text: windowRoot.rewindStatusText
                        color: "#00FF9D"
                        font.pixelSize: 10
                        font.bold: true
                    }

                    // Feature 2A: Time-Travel Checkpoint Button
                    Rectangle {
                        width: 22; height: 20; radius: 6
                        color: cpMouse.containsMouse ? "#1E293B" : "#151C28"
                        border.color: "#00F2FE"; border.width: 1
                        Text { anchors.centerIn: parent; text: "⏱"; font.pixelSize: 10; color: "#00F2FE" }
                        MouseArea {
                            id: cpMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                var t = (windowRoot.toplevel && windowRoot.toplevel.title) ? windowRoot.toplevel.title : "window";
                                SystemController.checkpointWindow(t);
                                windowRoot.rewindStatusText = "✓ Snapshot";
                            }
                        }
                    }

                    // Feature 2B: Time-Travel Rewind Button
                    Rectangle {
                        width: 22; height: 20; radius: 6
                        color: rwMouse.containsMouse ? "#1E293B" : "#151C28"
                        border.color: "#8B5CF6"; border.width: 1
                        Text { anchors.centerIn: parent; text: "⏪"; font.pixelSize: 10; color: "#D8B4FE" }
                        MouseArea {
                            id: rwMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                var t = (windowRoot.toplevel && windowRoot.toplevel.title) ? windowRoot.toplevel.title : "window";
                                SystemController.rewindWindow(t);
                                windowRoot.rewindStatusText = "⏪ Rewound";
                            }
                        }
                    }

                    // Feature 4: Gaze & Shoulder-Surfing Privacy Shield Toggle
                    Rectangle {
                        width: 22; height: 20; radius: 6
                        color: windowRoot.privacyShieldEnabled ? "#00F2FE" : "#151C28"
                        border.color: "#00F2FE"; border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: "🛡"
                            font.pixelSize: 10
                            color: windowRoot.privacyShieldEnabled ? "#06070A" : "#94A3B8"
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: windowRoot.privacyShieldEnabled = !windowRoot.privacyShieldEnabled
                        }
                    }

                    // Feature 1: Visual LiquidNode Port Connector
                    Rectangle {
                        width: 22; height: 20; radius: 10
                        color: (windowRoot.parent && windowRoot.parent.pendingNodeSource === windowRoot) ? "#00FF9D" : "#151C28"
                        border.color: "#00FF9D"; border.width: 1.5
                        Text {
                            anchors.centerIn: parent
                            text: "◉"
                            font.pixelSize: 11
                            color: (windowRoot.parent && windowRoot.parent.pendingNodeSource === windowRoot) ? "#06070A" : "#00FF9D"
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (windowRoot.parent && windowRoot.parent.handleNodePortClicked) {
                                    windowRoot.parent.handleNodePortClicked(windowRoot);
                                }
                            }
                        }
                    }
                }
            }

            // Drag Handler with Velocity Reset
            MouseArea {
                id: dragArea
                anchors.fill: parent
                drag.target: windowRoot
                drag.axis: Drag.XAndYAxis
                drag.minimumX: -windowRoot.width + 120
                drag.maximumX: (windowRoot.parent ? windowRoot.parent.width : 1920) - 120
                drag.minimumY: 36
                drag.maximumY: (windowRoot.parent ? windowRoot.parent.height : 1080) - 60

                onPressed: {
                    windowRoot.prevX = windowRoot.x;
                    windowRoot.prevY = windowRoot.y;
                    windowRoot.velX = 0;
                    windowRoot.velY = 0;
                    windowRoot.z = ++windowRoot.parent.topZ;
                    windowRoot.isFocusedWindow = true;
                    var t = (windowRoot.toplevel && windowRoot.toplevel.title) ? windowRoot.toplevel.title : "Application";
                    SystemController.boostFocusedWindow(t, false);
                }

                onReleased: {
                    windowRoot.velX = 0;
                    windowRoot.velY = 0;
                }
            }
        }

        // ---------------------------------------------------------------------
        // Native Wayland Client Surface Item
        // -------------------------------------------------------------------------
        ShellSurfaceItem {
            id: shellItem
            anchors.top: titleBar.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 4
            shellSurface: windowRoot.shellSurface
            opacity: (windowRoot.privacyShieldEnabled && (AuthManager.shoulderSurfingDetected || !windowRoot.isFocusedWindow)) ? 0.05 : 1.0
            onSurfaceDestroyed: windowRoot.destroy()
        }

        // Feature 4: Biometric Gaze & Shoulder-Surfing Frosted Privacy Veil
        Rectangle {
            id: privacyVeil
            anchors.top: titleBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 6
            radius: 10
            color: "#F20B0D14"
            border.color: "#00F2FE"
            border.width: 1
            visible: windowRoot.privacyShieldEnabled && (AuthManager.shoulderSurfingDetected || !windowRoot.isFocusedWindow)

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 8
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "🛡 BIOMETRIC PRIVACY SHIELD ACTIVE"
                    color: "#00F2FE"
                    font.bold: true
                    font.pixelSize: 13
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Window content frosted to prevent shoulder-surfing. Click or look at IR sensor to unblur."
                    color: "#94A3B8"
                    font.pixelSize: 11
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
