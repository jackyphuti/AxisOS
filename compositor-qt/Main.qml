import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtWayland.Compositor
import QtWayland.Compositor.XdgShell

WaylandCompositor {
    id: compositor

    Component.onCompleted: {
        // Automatically launch Welcome Center on first desktop startup
        SystemController.launchApp("if [ -f /var/lib/axisos/first-boot-pending ]; then axis-oobe; elif grep -q 'axisos.autoinstall=1' /proc/cmdline 2>/dev/null; then axis-installer; elif [ ! -f ~/.config/axisos/welcome-shown ]; then mkdir -p ~/.config/axisos && touch ~/.config/axisos/welcome-shown && axis-welcome; fi");
    }

    WaylandOutput {
        id: defaultOutput
        sizeFollowsWindow: true

        window: Window {
            id: rootWindow
            width: Screen.width > 0 ? Screen.width : 1280
            height: Screen.height > 0 ? Screen.height : 800
            visible: true
            visibility: Window.FullScreen
            title: "AxisOS - LiquidNodes Wayland Desktop"
            color: Theme.surface

            // Desktop Keyboard Shortcuts
            Shortcut {
                sequence: "Meta"
                onActivated: if (!AuthManager.isLocked) appLauncher.visible = !appLauncher.visible
            }
            Shortcut {
                sequence: "Meta+Return"
                onActivated: SystemController.launchApp("axis-terminal")
            }
            Shortcut {
                sequence: "Meta+T"
                onActivated: SystemController.launchApp("axis-terminal")
            }
            Shortcut {
                sequence: "Meta+E"
                onActivated: SystemController.launchApp("nautilus")
            }
            Shortcut {
                sequence: "Meta+L"
                onActivated: AuthManager.lockSession()
            }

            // 1. Hardware Accelerated Liquid Nebula & Plasma Nodes Background
            LiquidBackground {
                id: liquidBg
            }

            // 2. Desktop Workspace Surface Area + Visual LiquidNode Bezier Piping Graph
            Item {
                id: surfaceArea
                anchors.fill: parent
                anchors.topMargin: 36
                anchors.bottomMargin: 76
                property int topZ: 10
                property var pendingNodeSource: null
                property var nodeConnections: []

                MouseArea {
                    anchors.fill: parent
                    z: -1
                    onClicked: {
                        if (quickSettings.visible) quickSettings.visible = false;
                    }
                }

                function handleNodePortClicked(winObj) {
                    if (!pendingNodeSource) {
                        pendingNodeSource = winObj;
                    } else if (pendingNodeSource === winObj) {
                        pendingNodeSource = null;
                    } else {
                        var conn = { source: pendingNodeSource, target: winObj };
                        var updated = nodeConnections.slice();
                        updated.push(conn);
                        nodeConnections = updated;
                        var srcTitle = (pendingNodeSource.toplevel && pendingNodeSource.toplevel.title) ? pendingNodeSource.toplevel.title : "Window A";
                        var dstTitle = (winObj.toplevel && winObj.toplevel.title) ? winObj.toplevel.title : "Window B";
                        SystemController.connectLiquidNode(srcTitle, dstTitle);
                        pendingNodeSource = null;
                        nodeCableCanvas.requestPaint();
                    }
                }

                // Live Bezier Cable Renderer connecting piped windows
                Canvas {
                    id: nodeCableCanvas
                    anchors.fill: parent
                    z: 999
                    onPaint: {
                        var ctx = getContext("2d");
                        ctx.reset();
                        for (var i = 0; i < surfaceArea.nodeConnections.length; ++i) {
                            var link = surfaceArea.nodeConnections[i];
                            if (!link.source || !link.target) continue;

                            var x1 = link.source.x + link.source.width - 28;
                            var y1 = link.source.y + 17;
                            var x2 = link.target.x + link.target.width - 28;
                            var y2 = link.target.y + 17;
                            var dx = Math.abs(x2 - x1) * 0.45 + 60;

                            var grad = ctx.createLinearGradient(x1, y1, x2, y2);
                            grad.addColorStop(0.0, Theme.accent.toString());
                            grad.addColorStop(0.5, Theme.accentSoft.toString());
                            grad.addColorStop(1.0, Theme.success.toString());

                            ctx.strokeStyle = grad;
                            ctx.lineWidth = 3.0;
                            ctx.beginPath();
                            ctx.moveTo(x1, y1);
                            ctx.bezierCurveTo(x1 + dx, y1, x2 - dx, y2, x2, y2);
                            ctx.stroke();

                            // Endpoints
                            ctx.fillStyle = Theme.accent.toString();
                            ctx.beginPath();
                            ctx.arc(x1, y1, 5, 0, 2 * Math.PI);
                            ctx.fill();

                            ctx.fillStyle = Theme.success.toString();
                            ctx.beginPath();
                            ctx.arc(x2, y2, 5, 0, 2 * Math.PI);
                            ctx.fill();
                        }
                    }

                    Timer {
                        interval: 32
                        running: surfaceArea.nodeConnections.length > 0
                        repeat: true
                        onTriggered: nodeCableCanvas.requestPaint()
                    }
                }
            }

            // 3. Floating Glassmorphic Top Bar with D-Bus IPC System Telemetry
            LiquidTopBar {
                id: topBar
                visible: !AuthManager.isLocked
                onQuickSettingsRequested: quickSettings.visible = !quickSettings.visible
            }

            // Quick Settings / Control Center Popover (One UI Glassmorphic Card)
            QuickSettingsPanel {
                id: quickSettings
                anchors.top: topBar.bottom
                anchors.topMargin: 6
                anchors.right: parent.right
                anchors.rightMargin: 16
                z: 9000
            }

            // Freedesktop Desktop Notification Center (Daemon Integrated)
            NotificationPopup {
                id: notificationCenter
                z: 9100
            }

            // Hardware On-Screen Display Overlay (Volume / Brightness)
            OSDOverlay {
                id: osdOverlay
                z: 9200
            }

            // 4. Floating Glassmorphic Prism Dock with Papirus & AxisHarmony SVG Vector Icons
            LiquidDock {
                id: floatingDock
                visible: !AuthManager.isLocked
            }

            // 5. Glassmorphic Application Launcher Grid
            LiquidAppLauncher {
                id: appLauncher
                anchors.fill: parent
                visible: false
                z: 8500
            }

            // 6. Liquid Nodes Display Manager (Greeter / Lock Screen)
            LoginScreen {
                id: loginScreen
                anchors.fill: parent
            }
        }
    }

    // -------------------------------------------------------------------------
    // XdgShell Protocol Handling (Standard Desktop Wayland Client Apps)
    // -------------------------------------------------------------------------
    XdgShell {
        id: xdgShell

        onToplevelCreated: (toplevel, xdgSurface) => {
            var windowComponent = Qt.createComponent("GlassmorphicWindow.qml");
            if (windowComponent.status === Component.Ready) {
                windowComponent.createObject(surfaceArea, {
                    "shellSurface": xdgSurface,
                    "toplevel": toplevel
                });
            } else if (windowComponent.status === Component.Error) {
                console.error("[LiquidNodesOS] Failed to instantiate window:", windowComponent.errorString());
            }
        }
    }
}
