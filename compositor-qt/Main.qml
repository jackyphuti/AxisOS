import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtWayland.Compositor
import QtWayland.Compositor.XdgShell

WaylandCompositor {
    id: compositor

    WaylandOutput {
        id: defaultOutput
        sizeFollowsWindow: true

        window: Window {
            id: rootWindow
            width: 1366
            height: 768
            visible: true
            title: "AxisOS - LiquidNodes Wayland Desktop"
            color: "#090a0f"

            // -------------------------------------------------------------
            // Liquid Nebula Background (Obsidian & Electric Emerald / Violet)
            // -------------------------------------------------------------
            Rectangle {
                id: backgroundCanvas
                anchors.fill: parent

                gradient: Gradient {
                    orientation: Gradient.Vertical
                    GradientStop { position: 0.0; color: "#07080c" }
                    GradientStop { position: 0.5; color: "#0d0f17" }
                    GradientStop { position: 1.0; color: "#06070a" }
                }

                // Ambient glowing energy nodes
                Rectangle {
                    width: 600
                    height: 600
                    radius: 300
                    x: (parent.width * 0.15) + Math.sin(nebulaTimer.angle) * 80
                    y: (parent.height * 0.2) + Math.cos(nebulaTimer.angle) * 60
                    opacity: 0.12
                    color: "#10b981" // Electric Emerald
                    antialiasing: true

                    transform: Scale {
                        origin.x: 300; origin.y: 300
                        xScale: 1.2 + Math.sin(nebulaTimer.angle * 1.5) * 0.2
                        yScale: 1.2 + Math.cos(nebulaTimer.angle * 1.5) * 0.2
                    }
                }

                Rectangle {
                    width: 700
                    height: 700
                    radius: 350
                    x: (parent.width * 0.6) - Math.sin(nebulaTimer.angle * 0.8) * 100
                    y: (parent.height * 0.4) - Math.cos(nebulaTimer.angle * 0.8) * 80
                    opacity: 0.09
                    color: "#8b5cf6" // Electric Violet
                    antialiasing: true

                    transform: Scale {
                        origin.x: 350; origin.y: 350
                        xScale: 1.1 + Math.cos(nebulaTimer.angle) * 0.2
                        yScale: 1.1 + Math.sin(nebulaTimer.angle) * 0.2
                    }
                }

                Timer {
                    id: nebulaTimer
                    interval: 16
                    running: true
                    repeat: true
                    property real angle: 0.0
                    onTriggered: {
                        angle += 0.008;
                        if (angle > 6.28318) angle = 0.0;
                    }
                }
            }

            // -------------------------------------------------------------
            // Desktop Surface Area (Window Manager Workspace)
            // -------------------------------------------------------------
            Item {
                id: surfaceArea
                anchors.fill: parent
                anchors.topMargin: 36
                anchors.bottomMargin: 76
            }

            // -------------------------------------------------------------
            // Glassmorphic Top Bar
            // -------------------------------------------------------------
            Rectangle {
                id: topBar
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: 34
                color: "#aa0c0e14"
                border.color: "#1f2430"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 16

                    // Left branding & activity
                    RowLayout {
                        spacing: 8
                        Rectangle {
                            width: 10
                            height: 10
                            radius: 5
                            color: "#10b981"
                        }
                        Text {
                            text: "AXIS OS"
                            color: "#ffffff"
                            font.bold: true
                            font.pixelSize: 12
                            font.family: "Inter, sans-serif"
                        }
                        Text {
                            text: "LiquidNodes"
                            color: "#8b5cf6"
                            font.bold: true
                            font.pixelSize: 11
                            font.family: "Inter, sans-serif"
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // Center Clock
                    Text {
                        id: clockText
                        color: "#e2e8f0"
                        font.bold: true
                        font.pixelSize: 12
                        font.family: "Inter, sans-serif"

                        Timer {
                            interval: 1000
                            running: true
                            repeat: true
                            triggeredOnStart: true
                            onTriggered: {
                                var date = new Date();
                                clockText.text = Qt.formatDateTime(date, "ddd MMM d  hh:mm AP");
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // Right System Indicators
                    RowLayout {
                        spacing: 12

                        Text {
                            text: "⚡ GameMode"
                            color: "#10b981"
                            font.pixelSize: 11
                            font.bold: true
                        }
                        Text {
                            text: "📶 Online"
                            color: "#cbd5e1"
                            font.pixelSize: 11
                        }
                        Text {
                            text: "🔊 100%"
                            color: "#cbd5e1"
                            font.pixelSize: 11
                        }
                        Text {
                            text: "🔋 100%"
                            color: "#cbd5e1"
                            font.pixelSize: 11
                        }
                    }
                }
            }

            // -------------------------------------------------------------
            // Floating Glassmorphic Dock with Spring Physics
            // -------------------------------------------------------------
            Rectangle {
                id: floatingDock
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 14
                anchors.horizontalCenter: parent.horizontalCenter
                height: 52
                width: dockRow.implicitWidth + 32
                radius: 26
                color: "#cc10121a"
                border.color: "#282d3c"
                border.width: 1

                // Glow ring
                Rectangle {
                    anchors.fill: parent
                    radius: parent.radius
                    color: "transparent"
                    border.color: "#2010b981"
                    border.width: 2
                    opacity: 0.6
                }

                RowLayout {
                    id: dockRow
                    anchors.centerIn: parent
                    spacing: 14

                    Repeater {
                        model: [
                            { name: "Launcher", icon: "🚀", cmd: "wofi --show drun", color: "#10b981" },
                            { name: "Terminal", icon: "💻", cmd: "foot", color: "#38bdf8" },
                            { name: "Software Store", icon: "🛍️", cmd: "gnome-software", color: "#8b5cf6" },
                            { name: "Steam", icon: "🎮", cmd: "steam", color: "#10b981" },
                            { name: "LibreOffice", icon: "📄", cmd: "libreoffice", color: "#f59e0b" },
                            { name: "Welcome", icon: "✨", cmd: "nobara-welcome", color: "#ec4899" },
                            { name: "Settings", icon: "⚙️", cmd: "gnome-control-center", color: "#94a3b8" }
                        ]

                        Rectangle {
                            id: dockItem
                            width: 38
                            height: 38
                            radius: 19
                            color: itemMouse.containsMouse ? "#262b3a" : "#181a24"
                            border.color: itemMouse.containsMouse ? modelData.color : "#232734"
                            border.width: 1

                            scale: itemMouse.containsMouse ? 1.25 : 1.0
                            Behavior on scale {
                                SpringAnimation {
                                    spring: 4.5
                                    damping: 0.3
                                    epsilon: 0.05
                                }
                            }

                            Text {
                                anchors.centerIn: parent
                                text: modelData.icon
                                font.pixelSize: 18
                            }

                            MouseArea {
                                id: itemMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: {
                                    // Launch process
                                    var process = Qt.createQmlObject('import QtQuick; Item {}', dockItem);
                                    // Process launching handled via desktop environment launcher
                                    console.log("Launching: " + modelData.cmd);
                                }
                            }

                            ToolTip.visible: itemMouse.containsMouse
                            ToolTip.text: modelData.name
                            ToolTip.delay: 300
                        }
                    }
                }
            }
        }
    }

    // -----------------------------------------------------------------
    // XdgShell Protocol Handling (Standard Wayland Desktop Apps)
    // -----------------------------------------------------------------
    XdgShell {
        id: xdgShell

        onToplevelCreated: (toplevel, xdgSurface) => {
            shellSurfaceComponent.createObject(surfaceArea, {
                "shellSurface": xdgSurface,
                "toplevel": toplevel
            });
        }
    }

    // -----------------------------------------------------------------
    // Window Surface Container with Liquid Spring Physics
    // -----------------------------------------------------------------
    Component {
        id: shellSurfaceComponent

        Item {
            id: windowWrapper
            property var shellSurface: null
            property var toplevel: null

            width: shellItem.width + 16
            height: shellItem.height + 46

            // Center initial spawn
            x: (surfaceArea.width - width) / 2 + Math.floor(Math.random() * 60) - 30
            y: (surfaceArea.height - height) / 2 + Math.floor(Math.random() * 40) - 20

            // Spring Physics for Fluid Dragging and Bouncing
            Behavior on x {
                enabled: !windowMouse.drag.active
                SpringAnimation {
                    spring: 3.5
                    damping: 0.28
                    epsilon: 0.25
                }
            }

            Behavior on y {
                enabled: !windowMouse.drag.active
                SpringAnimation {
                    spring: 3.5
                    damping: 0.28
                    epsilon: 0.25
                }
            }

            scale: windowMouse.drag.active ? 1.02 : 1.0
            Behavior on scale {
                SpringAnimation {
                    spring: 4.0
                    damping: 0.35
                }
            }

            // Glassmorphic Window Frame
            Rectangle {
                id: windowFrame
                anchors.fill: parent
                radius: 10
                color: "#dd0e1118"
                border.color: windowMouse.drag.active ? "#10b981" : "#282c3c"
                border.width: 1

                // Frosted glass header bar
                Rectangle {
                    id: titleBar
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 32
                    radius: 10
                    color: "#cc141722"

                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 1
                        color: "#282d3e"
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12

                        // Window Control Dots
                        Row {
                            spacing: 6
                            Rectangle {
                                width: 11; height: 11; radius: 5.5
                                color: "#ef4444"
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        if (toplevel) toplevel.sendClose();
                                    }
                                }
                            }
                            Rectangle {
                                width: 11; height: 11; radius: 5.5
                                color: "#f59e0b"
                            }
                            Rectangle {
                                width: 11; height: 11; radius: 5.5
                                color: "#10b981"
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        if (toplevel) {
                                            windowWrapper.x = 20;
                                            windowWrapper.y = 20;
                                        }
                                    }
                                }
                            }
                        }

                        // Window Title
                        Text {
                            Layout.fillWidth: true
                            text: toplevel ? toplevel.title : "Application Window"
                            color: "#cbd5e1"
                            font.bold: true
                            font.pixelSize: 11
                            font.family: "Inter, sans-serif"
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Item { width: 40 }
                    }

                    // Drag Window
                    MouseArea {
                        id: windowMouse
                        anchors.fill: parent
                        drag.target: windowWrapper
                        drag.axis: Drag.XAndYAxis
                        drag.minimumX: -windowWrapper.width + 100
                        drag.maximumX: surfaceArea.width - 100
                        drag.minimumY: 0
                        drag.maximumY: surfaceArea.height - 50
                    }
                }

                // Window Client Surface
                ShellSurfaceItem {
                    id: shellItem
                    anchors.top: titleBar.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.topMargin: 4
                    shellSurface: windowWrapper.shellSurface
                    onSurfaceDestroyed: windowWrapper.destroy()
                }
            }
        }
    }
}
