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

    // Unified One UI Terminal Window Container
    Rectangle {
        id: windowFrame
        anchors.fill: parent
        radius: Theme.radiusLg
        color: Theme.base

        // Subtle 1px Border
        Rectangle {
            id: glassBorder
            anchors.fill: parent
            radius: parent.radius
            color: "transparent"
            border.width: 1
            border.color: terminalScreen.activeFocus ? Theme.accent : Theme.divider
            opacity: 0.85

            Behavior on border.color {
                ColorAnimation { duration: 180 }
            }
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Header Bar
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                color: Theme.surface
                radius: Theme.radiusLg

                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 12
                    color: parent.color
                }

                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 1
                    color: Theme.divider
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

                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: Theme.accent
                    }

                    Text {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        text: terminalBackend.title.length > 0 ? terminalBackend.title : "Terminal — " + terminalBackend.cols + " × " + terminalBackend.rows
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        font.family: "IBM Plex Sans, Inter, sans-serif"
                        color: Theme.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideMiddle
                    }

                    Rectangle {
                        Layout.preferredWidth: 64
                        Layout.preferredHeight: 20
                        radius: 10
                        color: Theme.accentSoft
                        border.width: 1
                        border.color: Theme.accent

                        Text {
                            anchors.centerIn: parent
                            text: terminalBackend.cols + " × " + terminalBackend.rows
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                            color: Theme.accent
                        }
                    }

                    RowLayout {
                        spacing: 4

                        Rectangle {
                            width: 24; height: 24; radius: 12
                            color: minMouse.containsMouse ? Theme.surfaceAlt : "transparent"
                            Text { anchors.centerIn: parent; text: "−"; font.pixelSize: 13; font.bold: true; color: Theme.textSecondary }
                            MouseArea {
                                id: minMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: rootWindow.showMinimized()
                            }
                        }

                        Rectangle {
                            width: 24; height: 24; radius: 12
                            color: maxMouse.containsMouse ? Theme.surfaceAlt : "transparent"
                            Text { anchors.centerIn: parent; text: "◻"; font.pixelSize: 11; color: Theme.textSecondary }
                            MouseArea {
                                id: maxMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: rootWindow.visibility === Window.Maximized ? rootWindow.showNormal() : rootWindow.showMaximized()
                            }
                        }

                        Rectangle {
                            width: 24; height: 24; radius: 12
                            color: clsMouse.containsMouse ? Theme.danger : "transparent"
                            Text { anchors.centerIn: parent; text: "✕"; font.pixelSize: 11; font.bold: true; color: clsMouse.containsMouse ? "#FFFFFF" : Theme.textSecondary }
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

            // Terminal Scene Graph Surface
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 8

                TerminalScreenItem {
                    id: terminalScreen
                    anchors.fill: parent
                    backend: terminalBackend
                    fontFamily: "IBM Plex Mono, DejaVu Sans Mono, monospace"
                    fontSize: 12
                    cursorBlink: true
                    focus: true

                    Component.onCompleted: {
                        terminalScreen.forceActiveFocus();
                    }
                }
            }

            // Offline "Self-Healing" Terminal Doctor Action Pill
            Rectangle {
                id: selfHealingPill
                Layout.fillWidth: true
                Layout.preferredHeight: terminalBackend.hasDiagnosticSuggestion ? 46 : 0
                Layout.leftMargin: 12
                Layout.rightMargin: 12
                Layout.bottomMargin: terminalBackend.hasDiagnosticSuggestion ? 10 : 0
                visible: terminalBackend.hasDiagnosticSuggestion
                radius: Theme.radiusSm
                color: Theme.surfaceAlt
                border.width: 1
                border.color: Theme.accent
                clip: true

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 12
                    spacing: 12

                    Text {
                        text: "🩺 " + terminalBackend.diagnosticTitle
                        color: Theme.accent
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "Suggested fix: " + terminalBackend.diagnosticFixCommand
                        color: Theme.textPrimary
                        font.pixelSize: 11
                        font.family: "monospace"
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        width: 108
                        height: 28
                        radius: 8
                        color: Theme.accent
                        Text {
                            anchors.centerIn: parent
                            text: "⚡ Fix & Retry"
                            color: "#FFFFFF"
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
                        color: Theme.surfaceElevated
                        Text {
                            anchors.centerIn: parent
                            text: "✕"
                            color: Theme.textSecondary
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
