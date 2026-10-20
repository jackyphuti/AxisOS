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

    // One UI Toned Terminal Window Container
    Rectangle {
        id: windowFrame
        anchors.fill: parent
        radius: 24
        color: "#1E1E2E"

        // Subtle 1px Border
        Rectangle {
            id: glassBorder
            anchors.fill: parent
            radius: parent.radius
            color: "transparent"
            border.width: 1
            border.color: terminalScreen.activeFocus ? "#7D95FF" : "#2E3349"
            opacity: 0.8

            Behavior on border.color {
                ColorAnimation { duration: 180 }
            }
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // -----------------------------------------------------------------
            // One UI Header Bar (Draggable, centered title, plain subtle buttons)
            // -----------------------------------------------------------------
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 38
                color: "#181825"
                radius: 24

                // Flatten bottom corners
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 12
                    color: parent.color
                }

                DragHandler {
                    target: rootWindow
                    grabPermissions: TapHandler.CanTakeOverFromItems
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 12

                    // Plain Centered Terminal Title
                    Text {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        text: terminalBackend.title.length > 0 ? terminalBackend.title : "Terminal — " + terminalBackend.cols + " × " + terminalBackend.rows
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        font.family: "Inter, -apple-system, system-ui, sans-serif"
                        color: "#F2F3F7"
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideMiddle
                    }

                    // Terminal Dimension / Status Badge
                    Rectangle {
                        Layout.preferredWidth: 64
                        Layout.preferredHeight: 20
                        radius: 10
                        color: "#2A3366"
                        border.width: 1
                        border.color: "#7D95FF"

                        Text {
                            anchors.centerIn: parent
                            text: terminalBackend.cols + " × " + terminalBackend.rows
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                            color: "#7D95FF"
                        }
                    }

                    // Window Control Icons (Plain Subtle Monochrome Buttons - No colored traffic lights)
                    RowLayout {
                        spacing: 4

                        // Minimize Button
                        Rectangle {
                            width: 24; height: 24; radius: 12
                            color: minMouse.containsMouse ? "#2E3349" : "transparent"
                            Text { anchors.centerIn: parent; text: "−"; font.pixelSize: 13; font.bold: true; color: "#9EA3B5" }
                            MouseArea {
                                id: minMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: rootWindow.showMinimized()
                            }
                        }

                        // Maximize Button
                        Rectangle {
                            width: 24; height: 24; radius: 12
                            color: maxMouse.containsMouse ? "#2E3349" : "transparent"
                            Text { anchors.centerIn: parent; text: "◻"; font.pixelSize: 11; color: "#9EA3B5" }
                            MouseArea {
                                id: maxMouse
                                anchors.fill: parent
                                hoverEnabled: true
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

                        // Close Button
                        Rectangle {
                            width: 24; height: 24; radius: 12
                            color: clsMouse.containsMouse ? "#E11D48" : "transparent"
                            Text { anchors.centerIn: parent; text: "✕"; font.pixelSize: 11; font.bold: true; color: clsMouse.containsMouse ? "#FFFFFF" : "#9EA3B5" }
                            MouseArea {
                                id: clsMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: rootWindow.close()
                            }
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

            // -----------------------------------------------------------------
            // Feature 6: Offline "Self-Healing" Terminal Doctor Action Pill
            // -----------------------------------------------------------------
            Rectangle {
                id: selfHealingPill
                Layout.fillWidth: true
                Layout.preferredHeight: terminalBackend.hasDiagnosticSuggestion ? 46 : 0
                Layout.leftMargin: 12
                Layout.rightMargin: 12
                Layout.bottomMargin: terminalBackend.hasDiagnosticSuggestion ? 10 : 0
                visible: terminalBackend.hasDiagnosticSuggestion
                radius: 12
                color: "#E6101826"
                border.width: 1
                border.color: "#00F2FE"
                clip: true

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 12
                    spacing: 12

                    Text {
                        text: "🩺 " + terminalBackend.diagnosticTitle
                        color: "#00F2FE"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "Suggested fix: " + terminalBackend.diagnosticFixCommand
                        color: "#E2E8F0"
                        font.pixelSize: 11
                        font.family: "monospace"
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        width: 108
                        height: 28
                        radius: 8
                        color: "#00F2FE"
                        Text {
                            anchors.centerIn: parent
                            text: "⚡ Fix & Retry"
                            color: "#06070A"
                            font.pixelSize: 11
                            font.bold: true
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                terminalBackend.applyDiagnosticFix();
                                terminalScreen.forceActiveFocus();
                            }
                        }
                    }

                    Rectangle {
                        width: 24
                        height: 24
                        radius: 6
                        color: "#1E293B"
                        Text {
                            anchors.centerIn: parent
                            text: "✕"
                            color: "#94A3B8"
                            font.pixelSize: 11
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: terminalBackend.dismissDiagnostic()
                        }
                    }
                }
            }
        }
    }
}
