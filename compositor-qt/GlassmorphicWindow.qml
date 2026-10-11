import QtQuick
import QtQuick.Layouts
import QtWayland.Compositor

Item {
    id: windowRoot
    property var shellSurface: null
    property var toplevel: null

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
                        color: "#F59E0B"
                    }

                    // Maximize / Snap Button
                    Rectangle {
                        width: 12; height: 12; radius: 6
                        color: "#10B981"

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                windowRoot.x = 24;
                                windowRoot.y = 44;
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

                Item { width: 36 }
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
            onSurfaceDestroyed: windowRoot.destroy()
        }
    }
}
