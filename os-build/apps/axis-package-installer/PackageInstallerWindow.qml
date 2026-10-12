import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

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
        color: "#E6080B10" // Deep obsidian glass
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

                    // Traffic Light Window Controls
                    RowLayout {
                        spacing: 8
                        Rectangle {
                            width: 12; height: 12; radius: 6
                            color: "#FF007A"
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: rootWindow.close() }
                        }
                        Rectangle {
                            width: 12; height: 12; radius: 6
                            color: "#FFD600"
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: rootWindow.showMinimized() }
                        }
                        Rectangle {
                            width: 12; height: 12; radius: 6
                            color: "#00FF9D"
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
                            color: "#F8FAFC"
                        }
                        Text {
                            text: "• Debian APT Application Manager"
                            font.pixelSize: 12
                            color: "#00F2FE"
                        }
                    }

                    // Status Badge
                    Rectangle {
                        Layout.preferredWidth: 120
                        Layout.preferredHeight: 24
                        radius: 12
                        color: "#16202E"
                        border.width: 1
                        border.color: PackageBackend.isBusy ? "#00F2FE" : "#283446"

                        Text {
                            anchors.centerIn: parent
                            text: PackageBackend.isBusy ? "Installing..." : "Ready"
                            font.pixelSize: 11
                            font.weight: Font.Medium
                            color: PackageBackend.isBusy ? "#00F2FE" : "#94A3B8"
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
                    color: "#A60C1017"
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
                            color: currentCategory === modelData ? "#243248" : (catMouse.containsMouse ? "#182232" : "transparent")
                            border.width: currentCategory === modelData ? 1 : 0
                            border.color: "#00F2FE"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 14
                                anchors.rightMargin: 14
                                spacing: 10

                                Text {
                                    text: modelData
                                    font.pixelSize: 13
                                    font.weight: currentCategory === modelData ? Font.DemiBold : Font.Normal
                                    color: currentCategory === modelData ? "#00F2FE" : "#E2E8F0"
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
                    color: "#1E2736"
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
                                color: modelData.selected ? "#1E2B3E" : (cardMouse.containsMouse ? "#141C28" : "#0F1622")
                                border.width: 1
                                border.color: modelData.selected ? "#00F2FE" : (modelData.installed ? "#1E3A2F" : "#1F2937")

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
                                                color: "#00F2FE"
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
                                                color: "#F8FAFC"
                                                elide: Text.ElideRight
                                            }

                                            Text {
                                                text: modelData.id
                                                font.pixelSize: 10
                                                color: "#64748B"
                                            }
                                        }

                                        // Status Pill or Checkbox
                                        Rectangle {
                                            Layout.preferredWidth: modelData.installed ? 64 : 22
                                            Layout.preferredHeight: 22
                                            radius: modelData.installed ? 6 : 11
                                            color: modelData.installed ? "#122E22" : (modelData.selected ? "#00F2FE" : "#1A2434")
                                            border.width: 1
                                            border.color: modelData.installed ? "#00FF9D" : (modelData.selected ? "#00F2FE" : "#334155")

                                            Text {
                                                anchors.centerIn: parent
                                                visible: modelData.installed
                                                text: "INSTALLED"
                                                font.pixelSize: 9
                                                font.weight: Font.Bold
                                                color: "#00FF9D"
                                            }

                                            Canvas {
                                                anchors.centerIn: parent
                                                width: 10; height: 10
                                                visible: !modelData.installed && modelData.selected
                                                onPaint: {
                                                    var ctx = getContext("2d"); ctx.reset();
                                                    ctx.strokeStyle = "#06070A"; ctx.lineWidth = 2.0;
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
                                        color: "#94A3B8"
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
                color: "#CC0B0F16"
                border.width: 1
                border.color: "#1E2736"

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
                        color: PackageBackend.selectedCount > 0 ? "#00F2FE" : "#64748B"
                    }

                    Item { Layout.fillWidth: true }

                    // Clear Button
                    Rectangle {
                        Layout.preferredWidth: 84
                        Layout.preferredHeight: 36
                        radius: 12
                        color: "#182232"
                        border.width: 1
                        border.color: "#28384E"
                        visible: PackageBackend.selectedCount > 0 && !PackageBackend.isBusy

                        Text {
                            anchors.centerIn: parent
                            text: "Clear"
                            font.pixelSize: 12
                            color: "#94A3B8"
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
                        color: PackageBackend.selectedCount > 0 && !PackageBackend.isBusy ? "#00F2FE" : "#1A2535"
                        border.width: 1
                        border.color: PackageBackend.selectedCount > 0 && !PackageBackend.isBusy ? "#00F2FE" : "#283446"
                        opacity: PackageBackend.selectedCount > 0 && !PackageBackend.isBusy ? 1.0 : 0.6

                        Text {
                            anchors.centerIn: parent
                            text: PackageBackend.isBusy ? "Installing..." : "Install Selected"
                            font.pixelSize: 12
                            font.weight: Font.Bold
                            color: PackageBackend.selectedCount > 0 && !PackageBackend.isBusy ? "#06070A" : "#64748B"
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
            color: "#F2080B10"
            border.width: 1
            border.color: "#00F2FE"
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
                        color: "#00F2FE"
                    }

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        Layout.preferredWidth: 70
                        Layout.preferredHeight: 28
                        radius: 8
                        color: "#1E2736"
                        visible: !PackageBackend.isBusy

                        Text {
                            anchors.centerIn: parent
                            text: "Close"
                            font.pixelSize: 11
                            color: "#E2E8F0"
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
                    color: "#05070B"
                    border.width: 1
                    border.color: "#182230"

                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: 8
                        clip: true

                        TextArea {
                            readOnly: true
                            text: PackageBackend.currentLog
                            font.family: "monospace"
                            font.pixelSize: 11
                            color: "#00FF9D"
                            wrapMode: Text.WrapAnywhere
                            background: null
                        }
                    }
                }
            }
        }
    }
}
