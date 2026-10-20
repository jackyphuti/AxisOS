import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: dockRoot
    anchors.bottom: parent.bottom
    anchors.bottomMargin: 16
    anchors.horizontalCenter: parent.horizontalCenter
    height: 60
    width: dockLayout.implicitWidth + 32
    radius: 999
    color: "#FFFFFF"
    border.color: "#E5E7EE"
    border.width: 1

    // Soft drop shadow without neon glow
    Rectangle {
        anchors.fill: parent
        anchors.topMargin: 4
        z: -1
        radius: 999
        color: "transparent"
        border.color: "#0F1428A0"
        border.width: 6
        opacity: 0.5
    }

    RowLayout {
        id: dockLayout
        anchors.centerIn: parent
        spacing: 10

        Repeater {
            model: [
                {
                    id: "launcher",
                    name: "Applications",
                    cmd: "__AXIS_LAUNCHER__",
                    icon: "file:///usr/share/icons/Papirus/48x48/apps/view-app-grid.svg",
                    accent: "#1428A0"
                },
                {
                    id: "terminal",
                    name: "Terminal",
                    cmd: "axis-terminal",
                    icon: "file:///usr/share/icons/Papirus/48x48/apps/utilities-terminal.svg",
                    accent: "#1428A0"
                },
                {
                    id: "welcome",
                    name: "AxisOS Welcome",
                    cmd: "axis-welcome",
                    icon: "file:///usr/share/icons/Papirus/48x48/apps/preferences-desktop.svg",
                    accent: "#1428A0"
                },
                {
                    id: "files",
                    name: "Files",
                    cmd: "nautilus",
                    icon: "file:///usr/share/icons/Papirus/48x48/apps/system-file-manager.svg",
                    accent: "#1428A0"
                },
                {
                    id: "browser",
                    name: "Web Browser",
                    cmd: "x-www-browser",
                    icon: "file:///usr/share/icons/Papirus/48x48/apps/chromium.svg",
                    accent: "#1428A0"
                },
                {
                    id: "packages",
                    name: "Package Installer",
                    cmd: "axis-package-installer",
                    icon: "file:///usr/share/icons/Papirus/48x48/apps/system-software-install.svg",
                    accent: "#1428A0"
                },
                {
                    id: "scheduler",
                    name: "Kernel Scheduler (SCX)",
                    cmd: "axis-scheduler-manager",
                    icon: "file:///usr/share/icons/Papirus/48x48/apps/preferences-system-performance.svg",
                    accent: "#1428A0"
                },
                {
                    id: "steam",
                    name: "Steam Games",
                    cmd: "steam",
                    icon: "file:///usr/share/icons/Papirus/48x48/apps/steam.svg",
                    accent: "#1428A0"
                },
                {
                    id: "tasks",
                    name: "Task Manager",
                    cmd: "axis-task-manager",
                    icon: "file:///usr/share/icons/Papirus/48x48/apps/utilities-system-monitor.svg",
                    accent: "#1428A0"
                },
                {
                    id: "settings",
                    name: "System Settings",
                    cmd: "axis-settings",
                    icon: "file:///usr/share/icons/Papirus/48x48/apps/preferences-system.svg",
                    accent: "#1428A0"
                },
                {
                    id: "installer",
                    name: "Install AxisOS",
                    cmd: "axis-installer",
                    icon: "file:///usr/share/icons/Papirus/48x48/apps/system-installer.svg",
                    accent: "#1428A0"
                }
            ]

            Rectangle {
                id: tile
                width: 44
                height: 44
                radius: 14
                color: mouseArea.containsMouse ? "#E6EBFF" : "#F4F6FB"
                border.color: mouseArea.containsMouse ? "#1428A0" : "#E5E7EE"
                border.width: 1

                // Subtle One UI 200ms scale on hover
                scale: mouseArea.pressed ? 0.94 : (mouseArea.containsMouse ? 1.08 : 1.0)
                y: mouseArea.containsMouse ? -4 : 0

                Behavior on scale {
                    NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
                }

                Behavior on y {
                    NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
                }

                Behavior on color {
                    ColorAnimation { duration: 180; easing.type: Easing.OutCubic }
                }

                Behavior on border.color {
                    ColorAnimation { duration: 180; easing.type: Easing.OutCubic }
                }

                // Vector app icon
                Image {
                    anchors.centerIn: parent
                    width: 26
                    height: 26
                    source: modelData.icon
                    sourceSize.width: 32
                    sourceSize.height: 32
                    smooth: true
                    mipmap: true
                    fillMode: Image.PreserveAspectFit

                    // Fallback visual 9-dot grid for launcher or missing icons
                    Grid {
                        anchors.centerIn: parent
                        columns: 3
                        spacing: 2.5
                        visible: parent.status !== Image.Ready || modelData.cmd === "__AXIS_LAUNCHER__"
                        Repeater {
                            model: 9
                            Rectangle {
                                width: 3.5
                                height: 3.5
                                radius: 1.75
                                color: "#1428A0"
                            }
                        }
                    }
                }

                // Active App Indicator Dot in Deep Blue (#1428A0), no glow
                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 3
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 4
                    height: 4
                    radius: 2
                    color: "#1428A0"
                    visible: modelData.id === "terminal" || modelData.id === "settings"
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (modelData.cmd === "__AXIS_LAUNCHER__") {
                            if (typeof appLauncher !== "undefined") {
                                appLauncher.visible = !appLauncher.visible;
                            }
                        } else {
                            SystemController.launchApp(modelData.cmd);
                        }
                    }
                }

                ToolTip.visible: mouseArea.containsMouse
                ToolTip.text: modelData.name
                ToolTip.delay: 200
            }
        }
    }
}
