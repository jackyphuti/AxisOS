import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: root
    visible: true
    width: 960
    height: 640
    minimumWidth: 840
    minimumHeight: 560
    title: qsTr("Welcome to AxisOS")
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint

    // Liquid Nodes Color Palette
    readonly property color cBg: "#0B0D13"
    readonly property color cCardBg: "#141722"
    readonly property color cCardHover: "#1C2030"
    readonly property color cBorder: "#23283B"
    readonly property color cBorderHover: "#00F2FE"
    readonly property color cAccentTeal: "#00F2FE"
    readonly property color cAccentViolet: "#9D4EDD"
    readonly property color cTextMain: "#F0F3F8"
    readonly property color cTextMuted: "#8E99AE"
    readonly property color cSuccess: "#10B981"

    // Window Dragging
    MouseArea {
        id: dragArea
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 64
        cursorShape: Qt.ArrowCursor
        property point clickPos: "0,0"

        onPressed: function(mouse) {
            clickPos = Qt.point(mouse.x, mouse.y)
        }
        onPositionChanged: function(mouse) {
            var delta = Qt.point(mouse.x - clickPos.x, mouse.y - clickPos.y)
            root.x += delta.x
            root.y += delta.y
        }
    }

    // Main Acrylic / Glass Panel
    Rectangle {
        anchors.fill: parent
        radius: 16
        color: root.cBg
        border.color: root.cBorder
        border.width: 1
        clip: true

        // Top Accent Glow Strip
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 2
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#00F2FE" }
                GradientStop { position: 0.5; color: "#9D4EDD" }
                GradientStop { position: 1.0; color: "#00F2FE" }
            }
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Header Bar
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 64
                Layout.leftMargin: 24
                Layout.rightMargin: 20
                Layout.topMargin: 8

                // AxisOS Official Vector Triangle Icon
                Image {
                    width: 36
                    height: 36
                    source: "file:///usr/share/icons/hicolor/scalable/apps/axisos.svg"
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                }

                ColumnLayout {
                    Layout.leftMargin: 12
                    spacing: 2
                    Text {
                        text: "AxisOS 2.0"
                        color: root.cTextMain
                        font.bold: true
                        font.pixelSize: 18
                        font.family: "Inter, Ubuntu, sans-serif"
                    }
                    Text {
                        text: "Liquid Nodes High-Performance Edition • XanMod Kernel"
                        color: root.cTextMuted
                        font.pixelSize: 11
                        font.family: "Inter, Ubuntu, sans-serif"
                    }
                }

                Item { Layout.fillWidth: true }

                // Live Session Badge
                Rectangle {
                    visible: WelcomeBackend.isLiveSession
                    height: 26
                    width: 120
                    radius: 13
                    color: "#2A1838"
                    border.color: root.cAccentViolet
                    border.width: 1

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 6
                        Rectangle {
                            width: 6; height: 6; radius: 3
                            color: root.cAccentViolet
                        }
                        Text {
                            text: "LIVE ENVIRONMENT"
                            color: "#D8B4FE"
                            font.pixelSize: 9
                            font.bold: true
                        }
                    }
                }

                // Close Button
                Rectangle {
                    width: 32
                    height: 32
                    radius: 8
                    color: closeHover.hovered ? "#EF4444" : "#1B2030"
                    Behavior on color { ColorAnimation { duration: 150 } }

                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        color: closeHover.hovered ? "#FFFFFF" : root.cTextMuted
                        font.pixelSize: 13
                    }

                    HoverHandler { id: closeHover }
                    TapHandler { onTapped: Qt.quit() }
                }
            }

            // Navigation Tabs
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 46
                Layout.leftMargin: 24
                Layout.rightMargin: 24
                spacing: 12

                property int currentTab: 0

                Repeater {
                    model: [
                        { name: "Hardware & Telemetry", icon: "📊" },
                        { name: "Quick Setup & Tuning", icon: "⚡" },
                        { name: "Install to Disk", icon: "💾" },
                        { name: "Community & Docs", icon: "🌐" }
                    ]

                    delegate: Rectangle {
                        height: 36
                        Layout.preferredWidth: 170
                        radius: 8
                        color: parent.currentTab === index ? root.cCardHover : "transparent"
                        border.color: parent.currentTab === index ? root.cBorderHover : "transparent"
                        border.width: 1

                        Behavior on color { ColorAnimation { duration: 150 } }

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 8
                            Text {
                                text: modelData.icon
                                font.pixelSize: 13
                            }
                            Text {
                                text: modelData.name
                                color: parent.parent.parent.currentTab === index ? root.cTextMain : root.cTextMuted
                                font.pixelSize: 12
                                font.bold: parent.parent.parent.currentTab === index
                            }
                        }

                        TapHandler {
                            onTapped: parent.parent.currentTab = index
                        }
                    }
                }

                Item { Layout.fillWidth: true }
            }

            // Divider Line
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: root.cBorder
            }

            // Content Area StackLayout
            StackLayout {
                id: contentStack
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 24
                currentIndex: navTabs.currentTab

                // TAB 0: Hardware & Telemetry
                ScrollView {
                    clip: true
                    ColumnLayout {
                        width: contentStack.width - 24
                        spacing: 16

                        Text {
                            text: "System Telemetry & Hardware Matrix"
                            color: root.cTextMain
                            font.bold: true
                            font.pixelSize: 16
                        }

                        GridLayout {
                            columns: 2
                            rowSpacing: 16
                            columnSpacing: 16
                            Layout.fillWidth: true

                            // CPU Card
                            Rectangle {
                                Layout.fillWidth: true
                                height: 110
                                radius: 12
                                color: root.cCardBg
                                border.color: root.cBorder

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 16
                                    RowLayout {
                                        Text { text: "CPU Processor"; color: root.cAccentTeal; font.bold: true; font.pixelSize: 12 }
                                        Item { Layout.fillWidth: true }
                                        Text { text: WelcomeBackend.cpuCores + " Cores"; color: root.cTextMuted; font.pixelSize: 11 }
                                    }
                                    Text {
                                        text: WelcomeBackend.cpuModel
                                        color: root.cTextMain
                                        font.bold: true
                                        font.pixelSize: 14
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                    Text {
                                        text: "Architecture: x86_64-v3 (Optimized AVX2 Instruction Set)"
                                        color: root.cTextMuted
                                        font.pixelSize: 11
                                    }
                                }
                            }

                            // GPU Card
                            Rectangle {
                                Layout.fillWidth: true
                                height: 110
                                radius: 12
                                color: root.cCardBg
                                border.color: root.cBorder

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 16
                                    RowLayout {
                                        Text { text: "GPU Graphics Accelerator"; color: root.cAccentViolet; font.bold: true; font.pixelSize: 12 }
                                        Item { Layout.fillWidth: true }
                                        Text { text: "Vulkan / Wayland"; color: root.cTextMuted; font.pixelSize: 11 }
                                    }
                                    Text {
                                        text: WelcomeBackend.gpuModel
                                        color: root.cTextMain
                                        font.bold: true
                                        font.pixelSize: 14
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                    Text {
                                        text: "Driver: Direct Rendering Manager (DRM / KMS)"
                                        color: root.cTextMuted
                                        font.pixelSize: 11
                                    }
                                }
                            }

                            // RAM Card
                            Rectangle {
                                Layout.fillWidth: true
                                height: 110
                                radius: 12
                                color: root.cCardBg
                                border.color: root.cBorder

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 16
                                    RowLayout {
                                        Text { text: "Physical RAM Memory"; color: root.cAccentTeal; font.bold: true; font.pixelSize: 12 }
                                        Item { Layout.fillWidth: true }
                                        Text { 
                                            text: WelcomeBackend.ramUsedGb.toFixed(1) + " GB / " + WelcomeBackend.ramTotalGb.toFixed(1) + " GB (" + WelcomeBackend.ramPercent + "%)"
                                            color: root.cTextMain
                                            font.bold: true
                                            font.pixelSize: 11
                                        }
                                    }
                                    // RAM Meter Bar
                                    Rectangle {
                                        Layout.fillWidth: true
                                        height: 8
                                        radius: 4
                                        color: "#23283B"
                                        Rectangle {
                                            width: parent.width * (WelcomeBackend.ramPercent / 100.0)
                                            height: parent.height
                                            radius: 4
                                            gradient: Gradient {
                                                orientation: Gradient.Horizontal
                                                GradientStop { position: 0.0; color: root.cAccentTeal }
                                                GradientStop { position: 1.0; color: root.cAccentViolet }
                                            }
                                        }
                                    }
                                    Text {
                                        text: "High-throughput DDR memory with Linux zero-copy pipes"
                                        color: root.cTextMuted
                                        font.pixelSize: 11
                                    }
                                }
                            }

                            // zRAM Card
                            Rectangle {
                                Layout.fillWidth: true
                                height: 110
                                radius: 12
                                color: root.cCardBg
                                border.color: root.cBorder

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 16
                                    RowLayout {
                                        Text { text: "zRAM Compressed Swap"; color: root.cSuccess; font.bold: true; font.pixelSize: 12 }
                                        Item { Layout.fillWidth: true }
                                        Text { 
                                            text: WelcomeBackend.zramActive ? (WelcomeBackend.zramUsedGb.toFixed(1) + " GB / " + WelcomeBackend.zramTotalGb.toFixed(1) + " GB") : "Disabled"
                                            color: WelcomeBackend.zramActive ? root.cSuccess : root.cTextMuted
                                            font.bold: true
                                            font.pixelSize: 11
                                        }
                                    }
                                    // zRAM Meter Bar
                                    Rectangle {
                                        Layout.fillWidth: true
                                        height: 8
                                        radius: 4
                                        color: "#23283B"
                                        Rectangle {
                                            width: WelcomeBackend.zramTotalGb > 0 ? (parent.width * (WelcomeBackend.zramUsedGb / WelcomeBackend.zramTotalGb)) : 0
                                            height: parent.height
                                            radius: 4
                                            color: root.cSuccess
                                        }
                                    }
                                    Text {
                                        text: "zstd hardware compression (2.8x memory capacity multiplier)"
                                        color: root.cTextMuted
                                        font.pixelSize: 11
                                    }
                                }
                            }
                        }

                        // System Environment Row
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            Rectangle {
                                Layout.fillWidth: true
                                height: 48
                                radius: 8
                                color: root.cCardBg
                                border.color: root.cBorder
                                RowLayout {
                                    anchors.centerIn: parent
                                    spacing: 8
                                    Text { text: "🐧 Kernel:"; color: root.cTextMuted; font.pixelSize: 12 }
                                    Text { text: WelcomeBackend.kernelVersion; color: root.cTextMain; font.bold: true; font.pixelSize: 12 }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                height: 48
                                radius: 8
                                color: root.cCardBg
                                border.color: root.cBorder
                                RowLayout {
                                    anchors.centerIn: parent
                                    spacing: 8
                                    Text { text: "🪟 Compositor:"; color: root.cTextMuted; font.pixelSize: 12 }
                                    Text { text: WelcomeBackend.displayServer; color: root.cTextMain; font.bold: true; font.pixelSize: 12 }
                                }
                            }
                        }
                    }
                }

                // TAB 1: Quick Setup & Tuning
                ScrollView {
                    clip: true
                    ColumnLayout {
                        width: contentStack.width - 24
                        spacing: 16

                        Text {
                            text: "System Setup & Performance Accelerators"
                            color: root.cTextMain
                            font.bold: true
                            font.pixelSize: 16
                        }

                        GridLayout {
                            columns: 2
                            rowSpacing: 16
                            columnSpacing: 16
                            Layout.fillWidth: true

                            // 1. Kernel Scheduler Card
                            Rectangle {
                                Layout.fillWidth: true
                                height: 110
                                radius: 12
                                color: schedHover.hovered ? root.cCardHover : root.cCardBg
                                border.color: schedHover.hovered ? root.cAccentTeal : root.cBorder

                                HoverHandler { id: schedHover }
                                TapHandler { onTapped: WelcomeBackend.launchApp("scheduler") }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 16
                                    spacing: 16

                                    Rectangle {
                                        width: 48; height: 48; radius: 10
                                        color: "#182A3A"
                                        Text { anchors.centerIn: parent; text: "⚡"; font.pixelSize: 22 }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 4
                                        Text { text: "Kernel Scheduler (SCX)"; color: root.cTextMain; font.bold: true; font.pixelSize: 14 }
                                        Text { 
                                            text: "Switch BORE, Lavd, or Rusty sched-ext schedulers on the fly."
                                            color: root.cTextMuted; font.pixelSize: 11; wrapMode: Text.WordWrap; Layout.fillWidth: true
                                        }
                                    }
                                }
                            }

                            // 2. Package Installer Card
                            Rectangle {
                                Layout.fillWidth: true
                                height: 110
                                radius: 12
                                color: pkgHover.hovered ? root.cCardHover : root.cCardBg
                                border.color: pkgHover.hovered ? root.cAccentTeal : root.cBorder

                                HoverHandler { id: pkgHover }
                                TapHandler { onTapped: WelcomeBackend.launchApp("installer") }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 16
                                    spacing: 16

                                    Rectangle {
                                        width: 48; height: 48; radius: 10
                                        color: "#2C1838"
                                        Text { anchors.centerIn: parent; text: "📦"; font.pixelSize: 22 }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 4
                                        Text { text: "Package Installer"; color: root.cTextMain; font.bold: true; font.pixelSize: 14 }
                                        Text { 
                                            text: "Curated software categories: Browsers, Gaming, Dev & Office."
                                            color: root.cTextMuted; font.pixelSize: 11; wrapMode: Text.WordWrap; Layout.fillWidth: true
                                        }
                                    }
                                }
                            }

                            // 3. System Settings Card
                            Rectangle {
                                Layout.fillWidth: true
                                height: 110
                                radius: 12
                                color: setHover.hovered ? root.cCardHover : root.cCardBg
                                border.color: setHover.hovered ? root.cAccentTeal : root.cBorder

                                HoverHandler { id: setHover }
                                TapHandler { onTapped: WelcomeBackend.launchApp("settings") }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 16
                                    spacing: 16

                                    Rectangle {
                                        width: 48; height: 48; radius: 10
                                        color: "#183228"
                                        Text { anchors.centerIn: parent; text: "⚙️"; font.pixelSize: 22 }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 4
                                        Text { text: "Control Center"; color: root.cTextMain; font.bold: true; font.pixelSize: 14 }
                                        Text { 
                                            text: "Configure Displays, Audio, Wi-Fi, BlueZ and Biometrics."
                                            color: root.cTextMuted; font.pixelSize: 11; wrapMode: Text.WordWrap; Layout.fillWidth: true
                                        }
                                    }
                                }
                            }

                            // 4. NVIDIA Auto-Installer
                            Rectangle {
                                Layout.fillWidth: true
                                height: 110
                                radius: 12
                                color: nvdHover.hovered ? root.cCardHover : root.cCardBg
                                border.color: nvdHover.hovered ? root.cAccentTeal : root.cBorder

                                HoverHandler { id: nvdHover }
                                TapHandler { onTapped: WelcomeBackend.launchApp("nvidia") }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 16
                                    spacing: 16

                                    Rectangle {
                                        width: 48; height: 48; radius: 10
                                        color: "#16331C"
                                        Text { anchors.centerIn: parent; text: "🎮"; font.pixelSize: 22 }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 4
                                        Text { text: "NVIDIA Drivers"; color: root.cTextMain; font.bold: true; font.pixelSize: 14 }
                                        Text { 
                                            text: "Detect PCI ID and install proprietary NVIDIA graphics modules."
                                            color: root.cTextMuted; font.pixelSize: 11; wrapMode: Text.WordWrap; Layout.fillWidth: true
                                        }
                                    }
                                }
                            }
                        }

                        // Performance Actions Row
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 16

                            // GameMode Button
                            Rectangle {
                                Layout.fillWidth: true
                                height: 52
                                radius: 8
                                color: WelcomeBackend.gameModeActive ? "#164E63" : root.cCardBg
                                border.color: WelcomeBackend.gameModeActive ? root.cAccentTeal : root.cBorder

                                RowLayout {
                                    anchors.centerIn: parent
                                    spacing: 12
                                    Text { text: "🎯 GameMode:"; color: root.cTextMuted; font.pixelSize: 12 }
                                    Text { 
                                        text: WelcomeBackend.gameModeActive ? "ACTIVE (Low Latency)" : "STANDBY (Click to toggle)"
                                        color: WelcomeBackend.gameModeActive ? root.cAccentTeal : root.cTextMain
                                        font.bold: true
                                        font.pixelSize: 12
                                    }
                                }

                                TapHandler { onTapped: WelcomeBackend.toggleGameMode() }
                            }

                            // zRAM Tuning Button
                            Rectangle {
                                Layout.fillWidth: true
                                height: 52
                                radius: 8
                                color: root.cCardBg
                                border.color: root.cBorder

                                RowLayout {
                                    anchors.centerIn: parent
                                    spacing: 12
                                    Text { text: "🚀 zRAM Tuning:"; color: root.cTextMuted; font.pixelSize: 12 }
                                    Text { text: "Apply XanMod Swappiness"; color: root.cSuccess; font.bold: true; font.pixelSize: 12 }
                                }

                                TapHandler { onTapped: WelcomeBackend.optimizeZram() }
                            }
                        }
                    }
                }

                // TAB 2: Install to Disk
                ScrollView {
                    clip: true
                    ColumnLayout {
                        width: contentStack.width - 24
                        spacing: 20

                        Text {
                            text: "Install AxisOS to Your System"
                            color: root.cTextMain
                            font.bold: true
                            font.pixelSize: 18
                        }

                        Text {
                            text: "AxisOS can be installed alongside existing operating systems or cleanly onto an entire NVMe/SSD drive with optimized BTRFS subvolumes, zRAM swap, and XanMod kernel configuration."
                            color: root.cTextMuted
                            font.pixelSize: 13
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 180
                            radius: 12
                            color: root.cCardBg
                            border.color: root.cAccentViolet
                            border.width: 1

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 24
                                spacing: 12

                                Text {
                                    text: "High-Performance Native OS Installer"
                                    color: root.cAccentViolet
                                    font.bold: true
                                    font.pixelSize: 16
                                }

                                Text {
                                    text: "• Automatic EFI system partition and BTRFS subvolume layout\n• Pre-configured zstd compression for ultra-fast disk I/O\n• XanMod real-time scheduling and low-latency audio stack"
                                    color: root.cTextMain
                                    font.pixelSize: 12
                                    lineHeight: 1.4
                                }

                                Item { Layout.fillHeight: true }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 44
                                    radius: 8
                                    gradient: Gradient {
                                        orientation: Gradient.Horizontal
                                        GradientStop { position: 0.0; color: root.cAccentTeal }
                                        GradientStop { position: 1.0; color: root.cAccentViolet }
                                    }

                                    Text {
                                        anchors.centerIn: parent
                                        text: "Launch AxisOS Disk Installer"
                                        color: "#05070A"
                                        font.bold: true
                                        font.pixelSize: 14
                                    }

                                    TapHandler {
                                        onTapped: WelcomeBackend.launchApp("os-installer")
                                    }
                                }
                            }
                        }
                    }
                }

                // TAB 3: Community & Docs
                ScrollView {
                    clip: true
                    ColumnLayout {
                        width: contentStack.width - 24
                        spacing: 16

                        Text {
                            text: "AxisOS Resources & Documentation"
                            color: root.cTextMain
                            font.bold: true
                            font.pixelSize: 16
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 90
                            radius: 12
                            color: root.cCardBg
                            border.color: root.cBorder

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 16

                                Text { text: "🐙"; font.pixelSize: 32 }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Text { text: "GitHub Source & Issue Tracker"; color: root.cTextMain; font.bold: true; font.pixelSize: 14 }
                                    Text { text: "Contribute code, report bugs, or follow development of AxisOS."; color: root.cTextMuted; font.pixelSize: 11 }
                                }

                                Rectangle {
                                    width: 120; height: 36; radius: 6
                                    color: root.cCardHover
                                    border.color: root.cAccentTeal
                                    Text { anchors.centerIn: parent; text: "Visit GitHub"; color: root.cAccentTeal; font.bold: true; font.pixelSize: 11 }
                                    TapHandler { onTapped: WelcomeBackend.openDocumentation() }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 90
                            radius: 12
                            color: root.cCardBg
                            border.color: root.cBorder

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 16

                                Text { text: "💻"; font.pixelSize: 32 }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Text { text: "Open High-Speed Terminal"; color: root.cTextMain; font.bold: true; font.pixelSize: 14 }
                                    Text { text: "Access Linux shell with libvterm, PTY master, and Scene Graph rendering."; color: root.cTextMuted; font.pixelSize: 11 }
                                }

                                Rectangle {
                                    width: 120; height: 36; radius: 6
                                    color: root.cCardHover
                                    border.color: root.cAccentViolet
                                    Text { anchors.centerIn: parent; text: "Open Terminal"; color: root.cAccentViolet; font.bold: true; font.pixelSize: 11 }
                                    TapHandler { onTapped: WelcomeBackend.launchApp("terminal") }
                                }
                            }
                        }
                    }
                }
            }

            // Footer Bar
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                Layout.leftMargin: 24
                Layout.rightMargin: 24
                Layout.bottomMargin: 8

                // Startup Checkbox
                RowLayout {
                    spacing: 8
                    Rectangle {
                        width: 18; height: 18; radius: 4
                        color: WelcomeBackend.showOnStartup ? root.cAccentTeal : root.cCardBg
                        border.color: root.cBorder

                        Text {
                            visible: WelcomeBackend.showOnStartup
                            anchors.centerIn: parent
                            text: "✓"
                            color: "#05070A"
                            font.bold: true
                            font.pixelSize: 11
                        }

                        TapHandler {
                            onTapped: WelcomeBackend.toggleStartup(!WelcomeBackend.showOnStartup)
                        }
                    }

                    Text {
                        text: "Show welcome screen on startup"
                        color: root.cTextMuted
                        font.pixelSize: 12
                        TapHandler {
                            onTapped: WelcomeBackend.toggleStartup(!WelcomeBackend.showOnStartup)
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                // Launch Web Browser
                Rectangle {
                    width: 130
                    height: 36
                    radius: 8
                    color: root.cCardBg
                    border.color: root.cBorder

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 6
                        Text { text: "🌐"; font.pixelSize: 12 }
                        Text { text: "Web Browser"; color: root.cTextMain; font.pixelSize: 12 }
                    }

                    TapHandler {
                        onTapped: WelcomeBackend.launchApp("browser")
                    }
                }

                // Close Button
                Rectangle {
                    width: 90
                    height: 36
                    radius: 8
                    color: root.cCardHover
                    border.color: root.cBorder

                    Text {
                        anchors.centerIn: parent
                        text: "Close"
                        color: root.cTextMain
                        font.pixelSize: 12
                    }

                    TapHandler {
                        onTapped: Qt.quit()
                    }
                }
            }
        }
    }

    property alias navTabs: navTabsObject
    QtObject {
        id: navTabsObject
        property int currentTab: 0
    }
}
