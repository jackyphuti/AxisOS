import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: notificationHost
    width: 360
    height: notifColumn.implicitHeight
    anchors.top: parent.top
    anchors.topMargin: 46
    anchors.right: parent.right
    anchors.rightMargin: 16
    z: 9500

    ListModel {
        id: notifModel
    }

    Connections {
        target: (typeof SystemController !== "undefined" && SystemController !== null) ? SystemController : null

        function onNotificationReceived(id, appName, summary, body, appIcon, timeout) {
            notifModel.insert(0, {
                "notifId": id,
                "appName": appName.length > 0 ? appName : "System",
                "summary": summary,
                "body": body,
                "appIcon": appIcon,
                "timeout": timeout > 0 ? timeout : 5000
            });
        }
    }

    Column {
        id: notifColumn
        width: parent.width
        spacing: 10

        Repeater {
            model: notifModel

            delegate: Rectangle {
                id: notifCard
                width: 360
                height: cardLayout.implicitHeight + 24
                radius: Theme.radiusMd
                color: Theme.surface
                border.color: Theme.divider
                border.width: 1
                clip: true

                // Slide in with spring physics
                x: 0
                scale: 1.0
                opacity: 1.0

                // Specular border
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 1
                    radius: Theme.radiusMd - 1
                    color: "transparent"
                    border.color: Theme.specularBorder
                    border.width: 1
                    z: 10
                }

                // Elevation drop shadow
                Rectangle {
                    anchors.fill: parent
                    anchors.topMargin: 4
                    radius: Theme.radiusMd
                    color: "transparent"
                    border.color: Theme.shadowColor
                    border.width: 8
                    opacity: 0.7
                    z: -1
                }

                ColumnLayout {
                    id: cardLayout
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 6

                    // App Header
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Rectangle {
                            width: 20
                            height: 20
                            radius: 10
                            color: Theme.accentSoft

                            Text {
                                anchors.centerIn: parent
                                text: model.appName.length > 0 ? model.appName[0].toUpperCase() : "A"
                                color: Theme.accent
                                font.bold: true
                                font.pixelSize: 10
                            }
                        }

                        Text {
                            text: model.appName
                            color: Theme.textSecondary
                            font.bold: true
                            font.pixelSize: 11
                            Layout.fillWidth: true
                        }

                        Text {
                            text: "now"
                            color: Theme.textSecondary
                            font.pixelSize: 10
                        }

                        // Close button
                        Text {
                            text: "✕"
                            color: Theme.textSecondary
                            font.pixelSize: 11

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: notifModel.remove(index)
                            }
                        }
                    }

                    // Summary
                    Text {
                        text: model.summary
                        color: Theme.textPrimary
                        font.bold: true
                        font.pixelSize: 13
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }

                    // Body
                    Text {
                        visible: model.body.length > 0
                        text: model.body
                        color: Theme.textSecondary
                        font.pixelSize: 12
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        maximumLineCount: 3
                        elide: Text.ElideRight
                    }
                }

                // Auto dismiss timer
                Timer {
                    interval: model.timeout
                    running: true
                    repeat: false
                    onTriggered: {
                        if (index >= 0 && index < notifModel.count) {
                            notifModel.remove(index);
                        }
                    }
                }
            }
        }
    }
}
