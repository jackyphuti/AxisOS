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
    radius: Theme.radiusPill
    color: Theme.surface
    border.color: Theme.divider
    border.width: 1

    // 1px inner specular border for contrast & legibility guards
    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Theme.radiusPill - 1
        color: "transparent"
        border.color: Theme.specularBorder
        border.width: 1
        z: 10
    }

    // Soft drop shadow without neon glow
    Rectangle {
        anchors.fill: parent
        anchors.topMargin: 4
        z: -1
        radius: Theme.radiusPill
        color: "transparent"
        border.color: Theme.shadowColor
        border.width: 8
        opacity: 0.6
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
                    icon: "file:///usr/share/icons/AxisHarmony/scalable/actions/view-app-grid-symbolic.svg"
                },
                {
                    id: "terminal",
                    name: "Terminal",
                    cmd: "axis-terminal",
                    icon: "file:///usr/share/icons/AxisHarmony/scalable/apps/utilities-terminal-symbolic.svg"
                },
                {
                    id: "welcome",
                    name: "AxisOS Welcome",
                    cmd: "axis-welcome",
                    icon: "file:///usr/share/icons/AxisHarmony/scalable/apps/preferences-system-symbolic.svg"
                },
                {
                    id: "files",
                    name: "Files",
                    cmd: "nautilus",
                    icon: "file:///usr/share/icons/AxisHarmony/scalable/apps/system-file-manager-symbolic.svg"
                },
                {
                    id: "browser",
                    name: "Web Browser",
                    cmd: "x-www-browser",
                    icon: "file:///usr/share/icons/AxisHarmony/scalable/apps/web-browser-symbolic.svg"
                },
                {
                    id: "packages",
                    name: "Package Installer",
                    cmd: "axis-package-installer",
                    icon: "file:///usr/share/icons/AxisHarmony/scalable/apps/system-software-install-symbolic.svg"
                },
                {
                    id: "scheduler",
                    name: "Kernel Scheduler (SCX)",
                    cmd: "axis-scheduler-manager",
                    icon: "file:///usr/share/icons/AxisHarmony/scalable/apps/preferences-system-symbolic.svg"
                },
                {
                    id: "steam",
                    name: "Steam Games",
                    cmd: "steam",
                    icon: "file:///usr/share/icons/AxisHarmony/scalable/apps/applications-games-symbolic.svg"
                },
                {
                    id: "tasks",
                    name: "Task Manager",
                    cmd: "axis-task-manager",
                    icon: "file:///usr/share/icons/AxisHarmony/scalable/apps/utilities-terminal-symbolic.svg"
                },
                {
                    id: "settings",
                    name: "Settings",
                    cmd: "axis-settings",
                    icon: "file:///usr/share/icons/AxisHarmony/scalable/apps/preferences-system-symbolic.svg"
                }
            ]

            Rectangle {
                id: dockItem
                width: 44
                height: 44
                radius: Theme.radiusMd
                color: mouseArea.containsMouse ? Theme.accentSoft : Theme.surfaceAlt
                border.color: mouseArea.containsMouse ? Theme.accent : Theme.divider
                border.width: 1

                // Interactive Spring Physics for hover and launch bounce
                property bool isBouncing: false
                scale: mouseArea.pressed ? 0.92 : (isBouncing ? 1.20 : (mouseArea.containsMouse ? 1.12 : 1.0))
                y: mouseArea.containsMouse ? -6 : 0

                Behavior on scale {
                    SpringAnimation { spring: Theme.springConstant; damping: Theme.springDamping; epsilon: Theme.springEpsilon }
                }

                Behavior on y {
                    SpringAnimation { spring: Theme.springConstant; damping: Theme.springDamping; epsilon: Theme.springEpsilon }
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
                    width: 24
                    height: 24
                    source: modelData.icon
                    sourceSize.width: 24
                    sourceSize.height: 24
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
                                color: Theme.accent
                            }
                        }
                    }
                }

                // Active App Indicator Dot in One UI Deep Blue, no neon glow
                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 3
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 4
                    height: 4
                    radius: 2
                    color: Theme.accent
                    visible: modelData.id === "terminal" || modelData.id === "settings"
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        dockItem.isBouncing = true;
                        bounceTimer.restart();

                        if (modelData.cmd === "__AXIS_LAUNCHER__") {
                            if (typeof appLauncher !== "undefined") {
                                appLauncher.visible = !appLauncher.visible;
                            }
                        } else {
                            SystemController.launchApp(modelData.cmd);
                        }
                    }
                }

                Timer {
                    id: bounceTimer
                    interval: 220
                    repeat: false
                    onTriggered: dockItem.isBouncing = false
                }

                ToolTip.visible: mouseArea.containsMouse
                ToolTip.text: modelData.name
                ToolTip.delay: 200
            }
        }
    }
}
