import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: launcherRoot
    anchors.fill: parent
    visible: false
    opacity: visible ? 1.0 : 0.0
    scale: visible ? 1.0 : 0.96

    Behavior on opacity {
        NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
    }
    Behavior on scale {
        SpringAnimation { spring: 4.5; damping: 0.35; epsilon: 0.01 }
    }

    onVisibleChanged: {
        if (visible) {
            searchInput.text = "";
            selectedCategory = "All";
            refreshApps();
            searchInput.forceActiveFocus();
        }
    }

    property string selectedCategory: "All"
    property var allAppsList: []
    property var filteredAppsList: []

    function refreshApps() {
        var raw = SystemController.getInstalledApps();
        allAppsList = raw;
        filterApps();
    }

    function filterApps() {
        var q = searchInput.text.toLowerCase().trim();
        var cat = selectedCategory;
        var result = [];

        for (var i = 0; i < allAppsList.length; i++) {
            var app = allAppsList[i];
            var nameMatch = app.name.toLowerCase().indexOf(q) !== -1;
            var execMatch = app.exec.toLowerCase().indexOf(q) !== -1;
            var catMatch = (cat === "All");

            if (!catMatch && app.categories) {
                var c = app.categories.toLowerCase();
                if (cat === "System" && (c.indexOf("system") !== -1 || c.indexOf("settings") !== -1)) catMatch = true;
                else if (cat === "Utilities" && (c.indexOf("utility") !== -1 || c.indexOf("accessories") !== -1)) catMatch = true;
                else if (cat === "Development" && (c.indexOf("development") !== -1 || c.indexOf("ide") !== -1)) catMatch = true;
                else if (cat === "Internet" && (c.indexOf("network") !== -1 || c.indexOf("web") !== -1)) catMatch = true;
                else if (cat === "Games" && (c.indexOf("game") !== -1 || c.indexOf("emulator") !== -1)) catMatch = true;
                else if (cat === "Media" && (c.indexOf("audio") !== -1 || c.indexOf("video") !== -1 || c.indexOf("graphics") !== -1)) catMatch = true;
            }

            if ((nameMatch || execMatch) && catMatch) {
                result.push(app);
            }
        }
        filteredAppsList = result;
    }

    // Semi-transparent backdrop blur
    Rectangle {
        anchors.fill: parent
        color: "#E606080E"

        MouseArea {
            anchors.fill: parent
            onClicked: launcherRoot.visible = false
        }
    }

    // Centered frosted glassmorphic launcher modal
    Rectangle {
        id: launcherModal
        width: Math.min(parent.width - 64, 880)
        height: Math.min(parent.height - 80, 680)
        anchors.centerIn: parent
        radius: 24
        color: "#D90F131D"
        border.color: "#28344A"
        border.width: 1

        // Top neon ambient highlight
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            radius: 24
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#00F2FE" }
                GradientStop { position: 0.5; color: "#8B5CF6" }
                GradientStop { position: 1.0; color: "#00FF9D" }
            }
            opacity: 0.8
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 28
            spacing: 18

            // 1. Header with Logo & Title
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Rectangle {
                    width: 32
                    height: 32
                    radius: 16
                    color: "#182030"
                    border.color: "#00F2FE"
                    border.width: 1.5

                    Text {
                        anchors.centerIn: parent
                        text: "▲"
                        color: "#00F2FE"
                        font.pixelSize: 14
                    }
                }

                ColumnLayout {
                    spacing: 2
                    Text {
                        text: "AxisOS Applications"
                        color: "#F8FAFC"
                        font.bold: true
                        font.pixelSize: 16
                        font.family: "Inter, Cantarell, sans-serif"
                    }
                    Text {
                        text: "Launch applications, system utilities, and configuration panels"
                        color: "#94A3B8"
                        font.pixelSize: 11
                    }
                }

                Item { Layout.fillWidth: true }

                // Close button
                Rectangle {
                    width: 28
                    height: 28
                    radius: 14
                    color: closeMouse.containsMouse ? "#EF4444" : "#1A2234"
                    border.color: "#28344A"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        color: "#FFFFFF"
                        font.pixelSize: 11
                        font.bold: true
                    }

                    MouseArea {
                        id: closeMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: launcherRoot.visible = false
                    }
                }
            }

            // 2. Search Field
            Rectangle {
                Layout.fillWidth: true
                height: 44
                radius: 12
                color: "#131824"
                border.color: searchInput.activeFocus ? "#00F2FE" : "#222C3E"
                border.width: searchInput.activeFocus ? 1.5 : 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    spacing: 10

                    Text {
                        text: "🔍"
                        font.pixelSize: 14
                        color: "#64748B"
                    }

                    TextInput {
                        id: searchInput
                        Layout.fillWidth: true
                        color: "#F8FAFC"
                        font.pixelSize: 13
                        font.family: "Inter, Cantarell, sans-serif"
                        verticalAlignment: TextInput.AlignVCenter
                        selectByMouse: true
                        clip: true

                        Text {
                            anchors.fill: parent
                            verticalAlignment: Text.AlignVCenter
                            text: "Type to search applications, utilities, settings..."
                            color: "#475569"
                            font.pixelSize: 13
                            visible: !searchInput.text && !searchInput.activeFocus
                        }

                        onTextChanged: launcherRoot.filterApps()

                        Keys.onEscapePressed: launcherRoot.visible = false
                        Keys.onReturnPressed: {
                            if (launcherRoot.filteredAppsList.length > 0) {
                                var topApp = launcherRoot.filteredAppsList[0];
                                SystemController.launchApp(topApp.exec);
                                launcherRoot.visible = false;
                            }
                        }
                    }

                    // Clear button
                    Text {
                        visible: searchInput.text.length > 0
                        text: "✕"
                        color: "#64748B"
                        font.pixelSize: 12
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: searchInput.text = ""
                        }
                    }
                }
            }

            // 3. Category Filter Tabs
            RowLayout {
                spacing: 8
                Repeater {
                    model: ["All", "System", "Utilities", "Development", "Internet", "Media", "Games"]
                    Rectangle {
                        height: 28
                        width: tabText.implicitWidth + 20
                        radius: 14
                        color: launcherRoot.selectedCategory === modelData ? "#00F2FE" : (tabMouse.containsMouse ? "#202A3C" : "#131824")
                        border.color: launcherRoot.selectedCategory === modelData ? "#00F2FE" : "#28344A"
                        border.width: 1

                        Text {
                            id: tabText
                            anchors.centerIn: parent
                            text: modelData
                            color: launcherRoot.selectedCategory === modelData ? "#06080E" : "#94A3B8"
                            font.pixelSize: 11
                            font.bold: launcherRoot.selectedCategory === modelData
                        }

                        MouseArea {
                            id: tabMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                launcherRoot.selectedCategory = modelData;
                                launcherRoot.filterApps();
                            }
                        }
                    }
                }
            }

            // 4. Application Grid
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                GridView {
                    id: appGridView
                    anchors.fill: parent
                    cellWidth: 156
                    cellHeight: 120
                    model: launcherRoot.filteredAppsList

                    delegate: Item {
                        width: appGridView.cellWidth
                        height: appGridView.cellHeight

                        Rectangle {
                            id: appTile
                            anchors.fill: parent
                            anchors.margins: 6
                            radius: 16
                            color: tileMouse.containsMouse ? "#222B3E" : "#151B28"
                            border.color: tileMouse.containsMouse ? "#00F2FE" : "#1F2738"
                            border.width: 1
                            scale: tileMouse.containsMouse ? 1.05 : 1.0

                            Behavior on scale {
                                SpringAnimation { spring: 5.0; damping: 0.35 }
                            }
                            Behavior on color {
                                ColorAnimation { duration: 120 }
                            }

                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 8
                                width: parent.width - 16

                                Image {
                                    Layout.alignment: Qt.AlignHCenter
                                    width: 44
                                    height: 44
                                    source: modelData.icon
                                    sourceSize.width: 48
                                    sourceSize.height: 48
                                    smooth: true
                                    mipmap: true
                                    fillMode: Image.PreserveAspectFit

                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: 24
                                        height: 24
                                        radius: 12
                                        color: "#00F2FE"
                                        visible: parent.status !== Image.Ready
                                    }
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.name
                                    color: tileMouse.containsMouse ? "#FFFFFF" : "#E2E8F0"
                                    font.pixelSize: 11
                                    font.bold: true
                                    font.family: "Inter, Cantarell, sans-serif"
                                    horizontalAlignment: Text.AlignHCenter
                                    elide: Text.ElideRight
                                    maximumLineCount: 1
                                }
                            }

                            MouseArea {
                                id: tileMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    SystemController.launchApp(modelData.exec);
                                    launcherRoot.visible = false;
                                }
                            }
                        }
                    }
                }
            }

            // 5. Quick Action Footer
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Text {
                    text: launcherRoot.filteredAppsList.length + " applications available"
                    color: "#64748B"
                    font.pixelSize: 11
                }

                Item { Layout.fillWidth: true }

                Repeater {
                    model: [
                        { name: "Terminal", cmd: "axis-terminal" },
                        { name: "Settings", cmd: "axis-settings" },
                        { name: "Task Manager", cmd: "axis-task-manager" },
                        { name: "Software", cmd: "axis-software-manager" }
                    ]

                    Rectangle {
                        height: 24
                        width: fText.implicitWidth + 16
                        radius: 6
                        color: fMouse.containsMouse ? "#202A3C" : "#131824"
                        border.color: "#28344A"
                        border.width: 1

                        Text {
                            id: fText
                            anchors.centerIn: parent
                            text: modelData.name
                            color: "#94A3B8"
                            font.pixelSize: 10
                            font.bold: true
                        }

                        MouseArea {
                            id: fMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                SystemController.launchApp(modelData.cmd);
                                launcherRoot.visible = false;
                            }
                        }
                    }
                }
            }
        }
    }
}
