import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import AxisTerminal

Window {
    id: rootWindow
    width: 960
    height: 580
    minimumWidth: 480
    minimumHeight: 320
    visible: true
    title: terminalBackend.title.length > 0 ? terminalBackend.title + " - AxisOS Terminal" : "AxisOS Terminal"
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint

    // Native C++ PTY & libvterm Backend
    TerminalBackend {
        id: terminalBackend
        Component.onCompleted: {
            startShell("/bin/bash");
        }
        onShellExited: (code) => {
            rootWindow.close();
        }
        onBellTriggered: {
            bellFlash.restart();
        }
    }

    // Visual Bell Flash
    SequentialAnimation {
        id: bellFlash
        NumberAnimation { target: glassBorder; property: "opacity"; to: 1.0; duration: 60 }
        NumberAnimation { target: glassBorder; property: "opacity"; to: 0.25; duration: 180 }
    }

    // Glassmorphic Prism Window Container
    Rectangle {
        id: windowFrame
        anchors.fill: parent
        radius: 16
        color: "#E606070A" // 90% Obsidian Base for frosted glass look

        // 1px Cyber-Teal / Electric-Violet Gradient Border
        Rectangle {
            id: glassBorder
            anchors.fill: parent
            radius: parent.radius
            color: "transparent"
            border.width: 1
            border.color: "#00F2FE"
            opacity: terminalScreen.activeFocus ? 0.6 : 0.25

            Behavior on opacity {
                NumberAnimation { duration: 200 }
            }
        }

        // Frosted Glass Shader Effect sampling background
        ShaderEffect {
            anchors.fill: parent
            opacity: 0.15
            fragmentShader: "
                #version 440
                layout(location = 0) in vec2 qt_TexCoord0;
                layout(location = 0) out vec4 fragColor;
                void main() {
                    vec2 uv = qt_TexCoord0;
                    float n = fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453);
                    fragColor = vec4(vec3(n * 0.04), 0.1);
                }
            "
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // -----------------------------------------------------------------
            // Glassmorphic Header Bar (Draggable)
            // -----------------------------------------------------------------
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 38
                color: "#CC0E141B"
                radius: 16

                // Flatten bottom corners
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 10
                    color: parent.color
                }

                DragHandler {
                    target: rootWindow
                    grabPermissions: TapHandler.CanTakeOverFromItems
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    spacing: 12

                    // Window Controls (Traffic Lights)
                    RowLayout {
                        spacing: 8

                        // Close Button (Neon Magenta)
                        Rectangle {
                            width: 12; height: 12; radius: 6
                            color: "#FF007A"
                            border.width: 1; border.color: "#CCFF007A"
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: rootWindow.close()
                            }
                        }

                        // Minimize Button (Cyber Amber)
                        Rectangle {
                            width: 12; height: 12; radius: 6
                            color: "#FFD600"
                            border.width: 1; border.color: "#CCFFD600"
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: rootWindow.showMinimized()
                            }
                        }

                        // Maximize / Restore Button (Cyber Mint)
                        Rectangle {
                            width: 12; height: 12; radius: 6
                            color: "#00FF9D"
                            border.width: 1; border.color: "#CC00FF9D"
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (rootWindow.visibility === Window.Maximized) {
                                        rootWindow.showNormal()
                                    } else {
                                        rootWindow.showMaximized()
                                    }
                                }
                            }
                        }
                    }

                    // Terminal Title
                    Text {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        text: terminalBackend.title.length > 0 ? terminalBackend.title : "bash - " + terminalBackend.cols + "x" + terminalBackend.rows
                        font.pixelSize: 12
                        font.weight: Font.Medium
                        color: "#94A3B8"
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideMiddle
                    }

                    // Terminal Dimension / Status Badge
                    Rectangle {
                        Layout.preferredWidth: 64
                        Layout.preferredHeight: 20
                        radius: 10
                        color: "#18212D"
                        border.width: 1
                        border.color: "#2EC4B6"

                        Text {
                            anchors.centerIn: parent
                            text: terminalBackend.cols + " × " + terminalBackend.rows
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                            color: "#00F2FE"
                        }
                    }
                }
            }

            // -----------------------------------------------------------------
            // Terminal Scene Graph Surface
            // -----------------------------------------------------------------
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 8

                // Hardware-accelerated Scene Graph Renderer
                TerminalScreenItem {
                    id: terminalScreen
                    anchors.fill: parent
                    backend: terminalBackend
                    fontFamily: "monospace"
                    fontSize: 12
                    cursorBlink: true
                    focus: true

                    Component.onCompleted: {
                        terminalScreen.forceActiveFocus();
                    }
                }
            }
        }
    }
}
