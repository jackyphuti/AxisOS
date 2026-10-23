import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import AxisSchedulerManager

Window {
    id: rootWindow
    width: 920
    height: 600
    minimumWidth: 700
    minimumHeight: 480
    visible: true
    title: "AxisOS Kernel Scheduler Manager"
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint

    // Glassmorphic Prism Window Frame
    Rectangle {
        id: windowFrame
        anchors.fill: parent
        radius: 18
        color: Theme.surface
        border.width: 1
        border.color: Theme.divider

        // Cyber-Teal Ambient Glow
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: "transparent"
            border.width: 1
            border.color: Theme.specularBorder
            opacity: 0.8
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // -----------------------------------------------------------------
            // Glassmorphic Header Bar (Draggable)
            // -----------------------------------------------------------------
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 46
                color: Theme.surfaceAlt
                radius: 18

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

                    RowLayout {
                        spacing: 8
                        Rectangle {
                            width: 12; height: 12; radius: 6; color: Theme.danger
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: rootWindow.close() }
                        }
                        Rectangle {
                            width: 12; height: 12; radius: 6; color: Theme.warning
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: rootWindow.showMinimized() }
                        }
                        Rectangle {
                            width: 12; height: 12; radius: 6; color: Theme.success
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: rootWindow.visibility === Window.Maximized ? rootWindow.showNormal() : rootWindow.showMaximized()
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        Text {
                            text: "Kernel Scheduler Manager"
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                            color: Theme.textPrimary
                        }
                        Text {
                            text: "• sched-ext (SCX) & XanMod Engine"
                            font.pixelSize: 12
                            color: Theme.accent
                        }
                    }

                    // Live Status Indicator
                    Rectangle {
                        Layout.preferredWidth: 140
                        Layout.preferredHeight: 24
                        radius: 12
                        color: Theme.surfaceElevated
                        border.width: 1
                        border.color: SchedulerBackend.isRunning ? Theme.accent : Theme.divider

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 6
                            Rectangle {
                                width: 8; height: 8; radius: 4
                                color: SchedulerBackend.isRunning ? Theme.success : Theme.textSecondary
                            }
                            Text {
                                text: SchedulerBackend.isRunning ? "SCX ACTIVE" : "STANDARD CFS"
                                font.pixelSize: 10
                                font.weight: Font.Bold
                                color: SchedulerBackend.isRunning ? Theme.success : Theme.textSecondary
                            }
                        }
                    }
                }
            }

            // -----------------------------------------------------------------
            // Telemetry & Hardware Cards
            // -----------------------------------------------------------------
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 16
                spacing: 12

                // Card 1: Active Scheduler
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 84
                    radius: 14
                    color: Theme.surfaceAlt
                    border.width: 1
                    border.color: Theme.divider

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 4
                        Text { text: "ACTIVE SCHEDULER"; font.pixelSize: 10; font.weight: Font.Bold; color: Theme.textSecondary }
                        Text {
                            text: SchedulerBackend.activeScheduler
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                            color: Theme.accent
                            elide: Text.ElideRight
                        }
                    }
                }

                // Card 2: CPU Frequency
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 84
                    radius: 14
                    color: Theme.surfaceAlt
                    border.width: 1
                    border.color: Theme.divider

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 4
                        Text { text: "CPU CLOCK FREQUENCY"; font.pixelSize: 10; font.weight: Font.Bold; color: Theme.textSecondary }
                        Text {
                            text: SchedulerBackend.cpuFreqGhz.toFixed(2) + " GHz"
                            font.pixelSize: 18
                            font.weight: Font.Bold
                            color: Theme.accent
                        }
                    }
                }

                // Card 3: CPU Load
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 84
                    radius: 14
                    color: Theme.surfaceAlt
                    border.width: 1
                    border.color: Theme.divider

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 4
                        Text { text: "CPU UTILIZATION"; font.pixelSize: 10; font.weight: Font.Bold; color: Theme.textSecondary }
                        RowLayout {
                            spacing: 8
                            Text {
                                text: SchedulerBackend.cpuUsagePercent + "%"
                                font.pixelSize: 18
                                font.weight: Font.Bold
                                color: Theme.success
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                height: 8
                                radius: 4
                                color: Theme.surfaceElevated
                                Rectangle {
                                    width: parent.width * (SchedulerBackend.cpuUsagePercent / 100.0)
                                    height: parent.height
                                    radius: 4
                                    color: Theme.success
                                    Behavior on width { NumberAnimation { duration: 250 } }
                                }
                            }
                        }
                    }
                }
            }

            // -----------------------------------------------------------------
            // CPU Governor Profile Bar
            // -----------------------------------------------------------------
            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.preferredHeight: 52
                radius: 12
                color: Theme.surfaceAlt
                border.width: 1
                border.color: Theme.divider

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 12

                    Text {
                        text: "CPU Scaling Governor:"
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        color: Theme.textPrimary
                    }

                    Repeater {
                        model: SchedulerBackend.availableGovernors

                        Rectangle {
                            Layout.preferredWidth: 110
                            Layout.fillHeight: true
                            radius: 8
                            color: SchedulerBackend.activeGovernor === modelData ? Theme.accent : (govMouse.containsMouse ? Theme.accentSoft : Theme.surfaceElevated)
                            border.width: 1
                            border.color: SchedulerBackend.activeGovernor === modelData ? Theme.accent : Theme.divider

                            Text {
                                anchors.centerIn: parent
                                text: modelData.toUpperCase()
                                font.pixelSize: 11
                                font.weight: Font.Bold
                                color: SchedulerBackend.activeGovernor === modelData ? "#FFFFFF" : Theme.textSecondary
                            }

                            MouseArea {
                                id: govMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: SchedulerBackend.setGovernor(modelData)
                            }
                        }
                    }
                }
            }

            // -----------------------------------------------------------------
            // Available Schedulers List
            // -----------------------------------------------------------------
            Text {
                Layout.leftMargin: 20
                Layout.topMargin: 14
                text: "Available Kernel Schedulers (sched-ext)"
                font.pixelSize: 13
                font.weight: Font.DemiBold
                color: Theme.textSecondary
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 16
                clip: true

                ListView {
                    anchors.fill: parent
                    spacing: 10
                    model: SchedulerBackend.availableSchedulers

                    delegate: Rectangle {
                        width: parent.width
                        height: 64
                        radius: 12
                        color: Theme.surfaceAlt
                        border.width: 1
                        border.color: SchedulerBackend.activeScheduler.startsWith(modelData.split(' ')[0]) ? Theme.accent : Theme.divider

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 16
                            spacing: 14

                            Rectangle {
                                width: 36; height: 36; radius: 18
                                color: Theme.surfaceElevated
                                border.width: 1
                                border.color: Theme.accent

                                Canvas {
                                    anchors.centerIn: parent
                                    width: 18; height: 18
                                    onPaint: {
                                        var ctx = getContext("2d"); ctx.reset();
                                        ctx.strokeStyle = Theme.accent.toString(); ctx.lineWidth = 1.8;
                                        // Speedometer / Gauge
                                        ctx.beginPath(); ctx.arc(9, 9, 7, 0.8 * Math.PI, 2.2 * Math.PI); ctx.stroke();
                                        ctx.beginPath(); ctx.moveTo(9, 9); ctx.lineTo(13, 5); ctx.stroke();
                                    }
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Text {
                                    text: modelData
                                    font.pixelSize: 13
                                    font.weight: Font.DemiBold
                                    color: Theme.textPrimary
                                }
                                Text {
                                    text: modelData.includes("lavd") ? "Gaming & strict latency control for smooth frame times" :
                                          modelData.includes("bpfland") ? "Optimized interactive latency for desktop multi-tasking" :
                                          modelData.includes("rusty") ? "High throughput parallel compilation and render scheduler" :
                                          "Custom eBPF sched-ext pluggable scheduler"
                                    font.pixelSize: 11
                                    color: Theme.textSecondary
                                }
                            }

                            // Action Button
                            Rectangle {
                                Layout.preferredWidth: 100
                                Layout.preferredHeight: 32
                                radius: 16
                                color: SchedulerBackend.activeScheduler.startsWith(modelData.split(' ')[0]) ?
                                       Theme.surfaceElevated : Theme.accentSoft
                                border.width: 1
                                border.color: SchedulerBackend.activeScheduler.startsWith(modelData.split(' ')[0]) ?
                                              Theme.success : Theme.accent

                                Text {
                                    anchors.centerIn: parent
                                    text: SchedulerBackend.activeScheduler.startsWith(modelData.split(' ')[0]) ?
                                          "ACTIVE" : "ACTIVATE"
                                    font.pixelSize: 11
                                    font.weight: Font.Bold
                                    color: SchedulerBackend.activeScheduler.startsWith(modelData.split(' ')[0]) ?
                                           Theme.success : Theme.accent
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: SchedulerBackend.startScheduler(modelData)
                                }
                            }
                        }
                    }
                }
            }

            // Bottom Bar: Status Message & Reset Button
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                color: Theme.surfaceAlt
                border.width: 1
                border.color: Theme.divider

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 20

                    Text {
                        text: SchedulerBackend.statusMessage.length > 0 ?
                              SchedulerBackend.statusMessage :
                              "XanMod kernel sched-ext ready."
                        font.pixelSize: 12
                        color: Theme.textSecondary
                    }

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        Layout.preferredWidth: 120
                        Layout.preferredHeight: 28
                        radius: 14
                        color: Theme.surfaceElevated
                        border.width: 1
                        border.color: Theme.divider

                        Text {
                            anchors.centerIn: parent
                            text: "Restore Default"
                            font.pixelSize: 11
                            color: Theme.textPrimary
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: SchedulerBackend.stopScheduler()
                        }
                    }
                }
            }
        }
    }
}
