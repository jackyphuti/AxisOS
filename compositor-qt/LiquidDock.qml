import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: dockRoot
    anchors.bottom: parent.bottom
    anchors.bottomMargin: 14
    anchors.horizontalCenter: parent.horizontalCenter
    height: 58
    width: dockLayout.implicitWidth + 36
    radius: 29
    color: "#D00C0E15" // Deep obsidian frosted glass
    border.color: "#283042" // Matte titanium border
    border.width: 1

    // Cyber-Teal / Electric Violet Neon Ambient Glow Ring
    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: "transparent"
        border.color: "#3300F2FE"
        border.width: 2
        opacity: 0.75
    }

    RowLayout {
        id: dockLayout
        anchors.centerIn: parent
        spacing: 12

        Repeater {
            model: [
                {
                    id: "terminal",
                    name: "Debian Terminal",
                    cmd: "axis-terminal",
                    icon: "file:///usr/share/icons/Papirus/48x48/apps/utilities-terminal.svg",
                    accent: "#00F2FE"
                },
                {
                    id: "files",
                    name: "File Manager",
                    cmd: "nautilus",
                    icon: "file:///usr/share/icons/Papirus/48x48/apps/system-file-manager.svg",
                    accent: "#38BDF8"
                },
                {
                    id: "browser",
                    name: "Web Browser",
                    cmd: "firefox",
                    icon: "file:///usr/share/icons/Papirus/48x48/apps/firefox.svg",
                    accent: "#F59E0B"
                },
                {
                    id: "software",
                    name: "App Center",
                    cmd: "axis-software-manager",
                    icon: "file:///usr/share/icons/Papirus/48x48/apps/system-software-install.svg",
                    accent: "#8B5CF6"
                },
                {
                    id: "steam",
                    name: "Steam & Games",
                    cmd: "steam",
                    icon: "file:///usr/share/icons/Papirus/48x48/apps/steam.svg",
                    accent: "#00F2FE"
                },
                {
                    id: "code",
                    name: "VS Code Editor",
                    cmd: "code",
                    icon: "file:///usr/share/icons/Papirus/48x48/apps/visual-studio-code.svg",
                    accent: "#38BDF8"
                },
                {
                    id: "tasks",
                    name: "Task Manager",
                    cmd: "axis-task-manager",
                    icon: "file:///usr/share/icons/Papirus/48x48/apps/utilities-system-monitor.svg",
                    accent: "#FF007F"
                },
                {
                    id: "settings",
                    name: "System Settings",
                    cmd: "axis-settings",
                    icon: "file:///usr/share/icons/Papirus/48x48/apps/preferences-system.svg",
                    accent: "#94A3B8"
                },
                {
                    id: "installer",
                    name: "Install AxisOS",
                    cmd: "axis-installer",
                    icon: "file:///usr/share/icons/Papirus/48x48/apps/system-installer.svg",
                    accent: "#10B981"
                }
            ]

            Rectangle {
                id: tile
                width: 42
                height: 42
                radius: 14
                color: mouseArea.containsMouse ? "#202534" : "#141722"
                border.color: mouseArea.containsMouse ? modelData.accent : "#222736"
                border.width: 1

                // Organic Spring Animation physics on hover
                scale: mouseArea.pressed ? 0.90 : (mouseArea.containsMouse ? 1.30 : 1.0)
                y: mouseArea.containsMouse ? -6 : 0

                Behavior on scale {
                    SpringAnimation {
                        spring: 5.0
                        damping: 0.32
                        epsilon: 0.02
                    }
                }

                Behavior on y {
                    SpringAnimation {
                        spring: 4.8
                        damping: 0.30
                        epsilon: 0.05
                    }
                }

                // Papirus SVG vector icon (Strictly no emojis!)
                Image {
                    anchors.centerIn: parent
                    width: 28
                    height: 28
                    source: modelData.icon
                    sourceSize.width: 32
                    sourceSize.height: 32
                    smooth: true
                    mipmap: true
                    fillMode: Image.PreserveAspectFit

                    // Fallback visual dot if icon path is unavailable
                    Rectangle {
                        anchors.centerIn: parent
                        width: 14
                        height: 14
                        radius: 7
                        color: modelData.accent
                        visible: parent.status !== Image.Ready
                    }
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        SystemController.launchApp(modelData.cmd);
                    }
                }

                ToolTip.visible: mouseArea.containsMouse
                ToolTip.text: modelData.name
                ToolTip.delay: 200
            }
        }
    }
}
