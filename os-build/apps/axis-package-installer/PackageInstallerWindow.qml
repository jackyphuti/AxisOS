import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import AxisPackageInstaller

Window {
    id: rootWindow
    width: 1040
    height: 680
    minimumWidth: 780
    minimumHeight: 520
    visible: true
    title: "AxisOS Package Installer"
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint

    property string currentCategory: "Browsers"

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

                    // Traffic Light Window Controls
                    RowLayout {
                        spacing: 8
                        Rectangle {
                            width: 12; height: 12; radius: 6
                            color: Theme.danger
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: rootWindow.close() }
                        }
                        Rectangle {
                            width: 12; height: 12; radius: 6
                            color: Theme.warning
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: rootWindow.showMinimized() }
                        }
                        Rectangle {
                            width: 12; height: 12; radius: 6
                            color: Theme.success
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: rootWindow.visibility === Window.Maximized ? rootWindow.showNormal() : rootWindow.showMaximized()
                            }
                        }
                    }

                    // Window Title & Subtitle
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 10

                        Text {
                            text: "Package Installer"
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                            color: Theme.textPrimary
                        }
                        Text {
                            text: "• Debian APT Application Manager"
                            font.pixelSize: 12
                            color: Theme.accent
                        }
                    }

                    // Status Badge
                    Rectangle {
                        Layout.preferredWidth: 120
                        Layout.preferredHeight: 24
                        radius: 12
                        color: Theme.surfaceElevated
                        border.width: 1
                        border.color: PackageBackend.isBusy ? Theme.accent : Theme.divider

                        Text {
                            anchors.centerIn: parent
                            text: PackageBackend.isBusy ? "Installing..." : "Ready"
                            font.pixelSize: 11
                            font.weight: Font.Medium
                            color: PackageBackend.isBusy ? Theme.accent : Theme.textSecondary
                        }
                    }
                }
            }

            // -----------------------------------------------------------------
            // Main Content Area: Sidebar + Package Cards
            // -----------------------------------------------------------------
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // Category Sidebar
                Rectangle {
                    Layout.preferredWidth: 200
                    Layout.fillHeight: true
                    color: Theme.base
                    border.width: 0

                    ListView {
                        id: categoryList
                        anchors.fill: parent
                        anchors.topMargin: 12
                        anchors.bottomMargin: 12
                        spacing: 4
                        model: PackageBackend.getCategories()
                        clip: true

                        delegate: Rectangle {
                            width: categoryList.width - 16
                            height: 40
                            anchors.horizontalCenter: parent.horizontalCenter
                            radius: 10
                            color: currentCategory === modelData ? Theme.accentSoft : (catMouse.containsMouse ? Theme.surfaceAlt : "transparent")
                            border.width: currentCategory === modelData ? 1 : 0
                            border.color: Theme.accent

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 14
                                anchors.rightMargin: 14
                                spacing: 10

                                Text {
                                    text: modelData
                                    font.pixelSize: 13
                                    font.weight: currentCategory === modelData ? Font.DemiBold : Font.Normal
                                    color: currentCategory === modelData ? Theme.accent : Theme.textPrimary
                                }
                            }

                            MouseArea {
                                id: catMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    currentCategory = modelData;
                                    packageGrid.model = PackageBackend.getPackagesForCategory(currentCategory);
                                }
                            }
                        }
                    }
                }

                // Vertical Divider
                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    color: Theme.divider
                }

                // Package Grid View
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: 16
                        clip: true

                        GridView {
                            id: packageGrid
                            anchors.fill: parent
                            cellWidth: 260
                            cellHeight: 140
                            model: PackageBackend.getPackagesForCategory(currentCategory)

                            Connections {
                                target: PackageBackend
                                function onSelectionChanged() {
                                    packageGrid.model = PackageBackend.getPackagesForCategory(currentCategory);
                                }
                                function onInstallFinished() {
                                    packageGrid.model = PackageBackend.getPackagesForCategory(currentCategory);
                                }
                            }

                            delegate: Rectangle {
                                width: 244
                                height: 124
                                radius: 14
                                color: modelData.selected ? Theme.accentSoft : (cardMouse.containsMouse ? Theme.surfaceElevated : Theme.surfaceAlt)
                                border.width: 1
                                border.color: modelData.selected ? Theme.accent : (modelData.installed ? Theme.success : Theme.divider)

                                Behavior on color { ColorAnimation { duration: 150 } }
                                Behavior on border.color { ColorAnimation { duration: 150 } }

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 12
                                    spacing: 8

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 10

                                        // App Icon
                                        Image {
                                            width: 32
                                            height: 32
                                            source: "file:///usr/share/icons/Papirus/48x48/apps/" + modelData.icon + ".svg"
                                            sourceSize.width: 32
                                            sourceSize.height: 32
                                            fillMode: Image.PreserveAspectFit

                                            Rectangle {
                                                anchors.centerIn: parent
                                                width: 14; height: 14; radius: 7
                                                color: Theme.accent
                                                visible: parent.status !== Image.Ready
                                            }
                                        }

                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 2

                                            Text {
                                                text: modelData.name
                                                font.pixelSize: 13
                                                font.weight: Font.DemiBold
                                                color: Theme.textPrimary
                                                elide: Text.ElideRight
                                            }

                                            Text {
                                                text: modelData.id
                                                font.pixelSize: 10
                                                color: Theme.textSecondary
                                            }
                                        }

                                        // Status Pill or Checkbox
                                        Rectangle {
                                            Layout.preferredWidth: modelData.installed ? 64 : 22
                                            Layout.preferredHeight: 22
                                            radius: modelData.installed ? 6 : 11
                                            color: modelData.installed ? Theme.surface : (modelData.selected ? Theme.accent : Theme.surfaceElevated)
                                            border.width: 1
                                            border.color: modelData.installed ? Theme.success : (modelData.selected ? Theme.accent : Theme.divider)

                                            Text {
                                                anchors.centerIn: parent
                                                visible: modelData.installed
                                                text: "INSTALLED"
                                                font.pixelSize: 9
                                                font.weight: Font.Bold
                                                color: Theme.success
                                            }

                                            Canvas {
                                                anchors.centerIn: parent
                                                width: 10; height: 10
                                                visible: !modelData.installed && modelData.selected
                                                onPaint: {
                                                    var ctx = getContext("2d"); ctx.reset();
                                                    ctx.strokeStyle = "#FFFFFF"; ctx.lineWidth = 2.0;
                                                    ctx.beginPath(); ctx.moveTo(1, 5); ctx.lineTo(4, 8); ctx.lineTo(9, 2); ctx.stroke();
                                                }
                                            }
                                        }
                                    }

                                    // Description
                                    Text {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        text: modelData.description
                                        font.pixelSize: 11
                                        color: Theme.textSecondary
                                        wrapMode: Text.WordWrap
                                        elide: Text.ElideRight
                                        maximumLineCount: 2
                                    }
                                }

                                MouseArea {
                                    id: cardMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: modelData.installed ? Qt.ArrowCursor : Qt.PointingHandCursor
                                    onClicked: {
                                        if (!modelData.installed) {
                                            PackageBackend.togglePackageSelection(modelData.id);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // -----------------------------------------------------------------
            // Bottom Action Bar
            // -----------------------------------------------------------------
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 62
                color: Theme.surfaceAlt
                border.width: 1
                border.color: Theme.divider

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 20

                    Text {
                        text: PackageBackend.selectedCount > 0 ?
                              PackageBackend.selectedCount + " package(s) selected" :
                              "Select packages to install"
                        font.pixelSize: 13
                        font.weight: Font.Medium
                        color: PackageBackend.selectedCount > 0 ? Theme.accent : Theme.textSecondary
                    }

                    Item { Layout.fillWidth: true }

                    // Clear Button
                    Rectangle {
                        Layout.preferredWidth: 84
                        Layout.preferredHeight: 36
                        radius: 12
                        color: Theme.surfaceElevated
                        border.width: 1
                        border.color: Theme.divider
                        visible: PackageBackend.selectedCount > 0 && !PackageBackend.isBusy

                        Text {
                            anchors.centerIn: parent
                            text: "Clear"
                            font.pixelSize: 12
                            color: Theme.textSecondary
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: PackageBackend.clearSelection()
                        }
                    }

                    // Install Selected Button
                    Rectangle {
                        Layout.preferredWidth: 160
                        Layout.preferredHeight: 36
                        radius: 18
                        color: PackageBackend.selectedCount > 0 && !PackageBackend.isBusy ? Theme.accent : Theme.surfaceElevated
                        border.width: 1
                        border.color: PackageBackend.selectedCount > 0 && !PackageBackend.isBusy ? Theme.accent : Theme.divider
                        opacity: PackageBackend.selectedCount > 0 && !PackageBackend.isBusy ? 1.0 : 0.6

                        Text {
                            anchors.centerIn: parent
                            text: PackageBackend.isBusy ? "Installing..." : "Install Selected"
                            font.pixelSize: 12
                            font.weight: Font.Bold
                            color: PackageBackend.selectedCount > 0 && !PackageBackend.isBusy ? "#FFFFFF" : Theme.textSecondary
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: PackageBackend.selectedCount > 0 && !PackageBackend.isBusy
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                progressDrawer.visible = true;
                                PackageBackend.installSelected();
                            }
                        }
                    }
                }
            }
        }

        // ---------------------------------------------------------------------
        // Live APT Installation Terminal Drawer
        // ---------------------------------------------------------------------
        Rectangle {
            id: progressDrawer
            anchors.bottom: parent.bottom
            width: parent.width
            height: parent.height * 0.55
            radius: 18
            color: Theme.surface
            border.width: 1
            border.color: Theme.accent
            visible: PackageBackend.isBusy

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: PackageBackend.statusMessage
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        color: Theme.accent
                    }

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        Layout.preferredWidth: 70
                        Layout.preferredHeight: 28
                        radius: 8
                        color: Theme.divider
                        visible: !PackageBackend.isBusy

                        Text {
                            anchors.centerIn: parent
                            text: "Close"
                            font.pixelSize: 11
                            color: Theme.textPrimary
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: progressDrawer.visible = false
                        }
                    }
                }

                // Log Output Terminal Window
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 10
                    color: Theme.base
                    border.width: 1
                    border.color: Theme.divider

                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: 8
                        clip: true

                        TextArea {
                            readOnly: true
                            text: PackageBackend.currentLog
                            font.family: "monospace"
                            font.pixelSize: 11
                            color: Theme.success
                            wrapMode: Text.WrapAnywhere
                            background: null
                        }
                    }
                }
            }
        }
    }
}
