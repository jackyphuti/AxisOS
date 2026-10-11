import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtWayland.Compositor
import QtWayland.Compositor.XdgShell

WaylandCompositor {
    id: compositor

    WaylandOutput {
        id: defaultOutput
        sizeFollowsWindow: true

        window: Window {
            id: rootWindow
            width: 1440
            height: 900
            visible: true
            title: "AxisOS - LiquidNodes Wayland Desktop"
            color: "#06070A"

            // 1. Hardware Accelerated Liquid Nebula & Plasma Nodes Background
            LiquidBackground {
                id: liquidBg
            }

            // 2. Desktop Workspace Surface Area for Wayland Windows
            Item {
                id: surfaceArea
                anchors.fill: parent
                anchors.topMargin: 36
                anchors.bottomMargin: 76
                property int topZ: 10
            }

            // 3. Floating Glassmorphic Top Bar with D-Bus IPC System Telemetry
            LiquidTopBar {
                id: topBar
                visible: !AuthManager.isLocked
            }

            // 4. Floating Glassmorphic Prism Dock with Papirus SVG Vector Icons
            LiquidDock {
                id: floatingDock
                visible: !AuthManager.isLocked
            }

            // 5. Liquid Nodes Display Manager (Greeter / Lock Screen)
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
