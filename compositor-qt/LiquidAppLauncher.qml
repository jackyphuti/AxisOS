import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: launcherRoot
    anchors.fill: parent
    visible: false
    opacity: visible ? 1.0 : 0.0
    scale: visible ? 1.0 : 0.98

    Behavior on opacity {
        NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
    }
    Behavior on scale {
        NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
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

    // Soft airy backdrop blur
    Rectangle {
        anchors.fill: parent
        color: "#801C1C1E"

        MouseArea {
            anchors.fill: parent
            onClicked: launcherRoot.visible = false
        }
    }

    // Centered One UI Launchpad Modal
    Rectangle {
        id: launcherModal
        width: Math.min(parent.width - 64, 880)
        height: Math.min(parent.height - 80, 680)
        anchors.centerIn: parent
        radius: 24
        color: "#FFFFFF"
        border.color: "#E5E7EE"
        border.width: 1

        // Soft elevation shadow
        Rectangle {
            anchors.fill: parent
            anchors.topMargin: 4
            z: -1
            radius: 24
            color: "transparent"
            border.color: "#0D1428A0"
            border.width: 10
            opacity: 0.8
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 28
            spacing: 18

            // 1. One UI Large Header
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                ColumnLayout {
                    spacing: 2
                    Text {
                        text: "Applications"
                        color: "#1C1C1E"
                        font.bold: true
                        font.pixelSize: 28
                        font.letterSpacing: -0.5
                        font.family: "Inter, -apple-system, system-ui, sans-serif"
                    }
                    Text {
                        text: "All installed software, system utilities, and configuration panels"
                        color: "#6B6F7B"
                        font.pixelSize: 13
                        font.family: "Inter, -apple-system, system-ui, sans-serif"
                    }
                }

                Item { Layout.fillWidth: true }

                // Close button
                Rectangle {
                    width: 32
                    height: 32
                    radius: 16
                    color: closeMouse.containsMouse ? "#E6EBFF" : "#F4F6FB"
                    border.color: "#E5E7EE"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        color: "#1C1C1E"
                        font.pixelSize: 12
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

            // 2. Search Input (Pill Shaped)
            Rectangle {
                Layout.fillWidth: true
                height: 46
                radius: 999
                color: "#F4F6FB"
                border.color: searchInput.activeFocus ? "#1428A0" : "#E5E7EE"
                border.width: searchInput.activeFocus ? 2 : 1

                Behavior on border.color { ColorAnimation { duration: 180; easing.type: Easing.OutCubic } }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 10

                    // Clean Vector Search Icon (Outline Magnifying Glass)
                    Canvas {
                        width: 16
                        height: 16
                        renderTarget: Canvas.FramebufferObject
                        onPaint: {
                            var ctx = getContext("2d");
                            ctx.reset();
                            ctx.strokeStyle = "#6B6F7B";
                            ctx.lineWidth = 1.75;
                            ctx.lineCap = "round";

                            // Glass circle
                            ctx.beginPath();
                            ctx.arc(6.5, 6.5, 4.5, 0, 2 * Math.PI);
                            ctx.stroke();

                            // Handle
                            ctx.beginPath();
                            ctx.moveTo(10, 10);
                            ctx.lineTo(14, 14);
                            ctx.stroke();
                        }
                    }

                    TextInput {
                        id: searchInput
                        Layout.fillWidth: true
                        color: "#1C1C1E"
                        font.pixelSize: 14
                        font.family: "Inter, -apple-system, system-ui, sans-serif"
                        verticalAlignment: TextInput.AlignVCenter
                        selectByMouse: true
                        clip: true

                        Text {
                            anchors.fill: parent
                            verticalAlignment: Text.AlignVCenter
                            text: "Search apps, tools, settings..."
                            color: "#9EA3B5"
                            font.pixelSize: 14
                            font.family: "Inter, -apple-system, system-ui, sans-serif"
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
                        color: "#6B6F7B"
                        font.pixelSize: 12
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: searchInput.text = ""
                        }
                    }
                }
            }

            // 3. Category Filter Tabs (Pill shaped)
            RowLayout {
                spacing: 8
                Repeater {
                    model: ["All", "System", "Utilities", "Development", "Internet", "Media", "Games"]
                    Rectangle {
                        height: 30
                        width: tabText.implicitWidth + 22
                        radius: 999
                        color: launcherRoot.selectedCategory === modelData ? "#1428A0" : (tabMouse.containsMouse ? "#E6EBFF" : "#F4F6FB")
                        border.color: launcherRoot.selectedCategory === modelData ? "#1428A0" : "#E5E7EE"
                        border.width: 1

                        Behavior on color { ColorAnimation { duration: 180; easing.type: Easing.OutCubic } }

                        Text {
                            id: tabText
                            anchors.centerIn: parent
                            text: modelData
                            color: launcherRoot.selectedCategory === modelData ? "#FFFFFF" : "#1C1C1E"
                            font.pixelSize: 12
                            font.bold: launcherRoot.selectedCategory === modelData
                            font.family: "Inter, -apple-system, system-ui, sans-serif"
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
                    cellHeight: 124
                    model: launcherRoot.filteredAppsList

                    delegate: Item {
                        width: appGridView.cellWidth
                        height: appGridView.cellHeight

                        Rectangle {
                            id: appTile
                            anchors.fill: parent
                            anchors.margins: 6
                            radius: 16
                            color: tileMouse.containsMouse ? "#E6EBFF" : "#F4F6FB"
                            border.color: tileMouse.containsMouse ? "#1428A0" : "#E5E7EE"
                            border.width: 1
                            scale: tileMouse.containsMouse ? 1.04 : 1.0

                            Behavior on scale {
                                NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
                            }
                            Behavior on color {
                                ColorAnimation { duration: 180; easing.type: Easing.OutCubic }
                            }
                            Behavior on border.color {
                                ColorAnimation { duration: 180; easing.type: Easing.OutCubic }
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
                                        width: 20
                                        height: 20
                                        radius: 10
                                        color: "#1428A0"
                                        visible: parent.status !== Image.Ready
                                    }
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.name
                                    color: "#1C1C1E"
                                    font.pixelSize: 12
                                    font.bold: true
                                    font.family: "Inter, -apple-system, system-ui, sans-serif"
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
                    text: launcherRoot.filteredAppsList.length + " applications"
                    color: "#6B6F7B"
                    font.pixelSize: 12
                    font.family: "Inter, -apple-system, system-ui, sans-serif"
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
                        height: 28
                        width: fText.implicitWidth + 18
                        radius: 999
                        color: fMouse.containsMouse ? "#E6EBFF" : "#F4F6FB"
                        border.color: "#E5E7EE"
                        border.width: 1

                        Behavior on color { ColorAnimation { duration: 180; easing.type: Easing.OutCubic } }

                        Text {
                            id: fText
                            anchors.centerIn: parent
                            text: modelData.name
                            color: "#1C1C1E"
                            font.pixelSize: 11
                            font.bold: true
                            font.family: "Inter, -apple-system, system-ui, sans-serif"
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
