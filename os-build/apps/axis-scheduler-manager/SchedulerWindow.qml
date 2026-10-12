import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

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
        color: "#E6080B10"
        border.width: 1
        border.color: "#283446"

        // Cyber-Teal Ambient Glow
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: "transparent"
            border.width: 1
            border.color: "#3300F2FE"
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
                color: "#CC0E141E"
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
                            width: 12; height: 12; radius: 6; color: "#FF007A"
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: rootWindow.close() }
                        }
                        Rectangle {
                            width: 12; height: 12; radius: 6; color: "#FFD600"
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: rootWindow.showMinimized() }
                        }
                        Rectangle {
                            width: 12; height: 12; radius: 6; color: "#00FF9D"
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
                            color: "#F8FAFC"
                        }
                        Text {
                            text: "• sched-ext (SCX) & XanMod Engine"
                            font.pixelSize: 12
                            color: "#00F2FE"
                        }
                    }

                    // Live Status Indicator
                    Rectangle {
                        Layout.preferredWidth: 140
                        Layout.preferredHeight: 24
                        radius: 12
                        color: "#16202E"
                        border.width: 1
                        border.color: SchedulerBackend.isRunning ? "#00F2FE" : "#334155"

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 6
                            Rectangle {
                                width: 8; height: 8; radius: 4
                                color: SchedulerBackend.isRunning ? "#00FF9D" : "#64748B"
                            }
                            Text {
                                text: SchedulerBackend.isRunning ? "SCX ACTIVE" : "STANDARD CFS"
                                font.pixelSize: 10
                                font.weight: Font.Bold
                                color: SchedulerBackend.isRunning ? "#00FF9D" : "#94A3B8"
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
                    color: "#0F1622"
                    border.width: 1
                    border.color: "#243248"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 4
                        Text { text: "ACTIVE SCHEDULER"; font.pixelSize: 10; font.weight: Font.Bold; color: "#64748B" }
                        Text {
                            text: SchedulerBackend.activeScheduler
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                            color: "#00F2FE"
                            elide: Text.ElideRight
                        }
                    }
                }

                // Card 2: CPU Frequency
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 84
                    radius: 14
                    color: "#0F1622"
                    border.width: 1
                    border.color: "#243248"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 4
                        Text { text: "CPU CLOCK FREQUENCY"; font.pixelSize: 10; font.weight: Font.Bold; color: "#64748B" }
                        Text {
                            text: SchedulerBackend.cpuFreqGhz.toFixed(2) + " GHz"
                            font.pixelSize: 18
                            font.weight: Font.Bold
                            color: "#7928CA"
                        }
                    }
                }

                // Card 3: CPU Load
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 84
                    radius: 14
                    color: "#0F1622"
                    border.width: 1
                    border.color: "#243248"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 4
                        Text { text: "CPU UTILIZATION"; font.pixelSize: 10; font.weight: Font.Bold; color: "#64748B" }
                        RowLayout {
                            spacing: 8
                            Text {
                                text: SchedulerBackend.cpuUsagePercent + "%"
                                font.pixelSize: 18
                                font.weight: Font.Bold
                                color: "#00FF9D"
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                height: 8
                                radius: 4
                                color: "#16202E"
                                Rectangle {
                                    width: parent.width * (SchedulerBackend.cpuUsagePercent / 100.0)
                                    height: parent.height
                                    radius: 4
                                    color: "#00FF9D"
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
                color: "#0E141E"
                border.width: 1
                border.color: "#222D3E"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 12

                    Text {
                        text: "CPU Scaling Governor:"
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        color: "#E2E8F0"
                    }

                    Repeater {
                        model: SchedulerBackend.availableGovernors

                        Rectangle {
                            Layout.preferredWidth: 110
                            Layout.fillHeight: true
                            radius: 8
                            color: SchedulerBackend.activeGovernor === modelData ? "#00F2FE" : (govMouse.containsMouse ? "#1E2B3C" : "#141D2A")
                            border.width: 1
                            border.color: SchedulerBackend.activeGovernor === modelData ? "#00F2FE" : "#2E3E56"

                            Text {
                                anchors.centerIn: parent
                                text: modelData.toUpperCase()
                                font.pixelSize: 11
                                font.weight: Font.Bold
                                color: SchedulerBackend.activeGovernor === modelData ? "#06070A" : "#94A3B8"
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
                color: "#94A3B8"
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
                        color: "#0F1622"
                        border.width: 1
                        border.color: SchedulerBackend.activeScheduler.startsWith(modelData.split(' ')[0]) ? "#00F2FE" : "#1E293B"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 16
                            spacing: 14

                            Rectangle {
                                width: 36; height: 36; radius: 18
                                color: "#16202E"
                                border.width: 1
                                border.color: "#00F2FE"

                                Canvas {
                                    anchors.centerIn: parent
                                    width: 18; height: 18
                                    onPaint: {
                                        var ctx = getContext("2d"); ctx.reset();
                                        ctx.strokeStyle = "#00F2FE"; ctx.lineWidth = 1.8;
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
                                    color: "#F8FAFC"
                                }
                                Text {
                                    text: modelData.includes("lavd") ? "Gaming & strict latency control for smooth frame times" :
                                          modelData.includes("bpfland") ? "Optimized interactive latency for desktop multi-tasking" :
                                          modelData.includes("rusty") ? "High throughput parallel compilation and render scheduler" :
                                          "Custom eBPF sched-ext pluggable scheduler"
                                    font.pixelSize: 11
                                    color: "#64748B"
                                }
                            }

                            // Action Button
                            Rectangle {
                                Layout.preferredWidth: 100
                                Layout.preferredHeight: 32
                                radius: 16
                                color: SchedulerBackend.activeScheduler.startsWith(modelData.split(' ')[0]) ?
                                       "#122E22" : "#1A2536"
                                border.width: 1
                                border.color: SchedulerBackend.activeScheduler.startsWith(modelData.split(' ')[0]) ?
                                              "#00FF9D" : "#00F2FE"

                                Text {
                                    anchors.centerIn: parent
                                    text: SchedulerBackend.activeScheduler.startsWith(modelData.split(' ')[0]) ?
                                          "ACTIVE" : "ACTIVATE"
                                    font.pixelSize: 11
                                    font.weight: Font.Bold
                                    color: SchedulerBackend.activeScheduler.startsWith(modelData.split(' ')[0]) ?
                                           "#00FF9D" : "#00F2FE"
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
                color: "#CC0B0F16"
                border.width: 1
                border.color: "#1E2736"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 20

                    Text {
                        text: SchedulerBackend.statusMessage.length > 0 ?
                              SchedulerBackend.statusMessage :
                              "XanMod kernel sched-ext ready."
                        font.pixelSize: 12
                        color: "#94A3B8"
                    }

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        Layout.preferredWidth: 120
                        Layout.preferredHeight: 28
                        radius: 14
                        color: "#1A202E"
                        border.width: 1
                        border.color: "#334155"

                        Text {
                            anchors.centerIn: parent
                            text: "Restore Default"
                            font.pixelSize: 11
                            color: "#E2E8F0"
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
