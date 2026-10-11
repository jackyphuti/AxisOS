import QtQuick

Item {
    id: root
    anchors.fill: parent

    // Deep Obsidian Base Layer
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: "#06070A" }
            GradientStop { position: 0.45; color: "#0A0B10" }
            GradientStop { position: 0.85; color: "#08090E" }
            GradientStop { position: 1.0; color: "#050608" }
        }
    }

    // -------------------------------------------------------------------------
    // Liquid Nodes: Orbital Neon Plasma Energy Fields
    // (Cyber-Teal, Magenta, and Electric Violet)
    // -------------------------------------------------------------------------

    // 1. Cyber-Teal Liquid Energy Node (#00F2FE)
    Rectangle {
        id: tealNode
        width: Math.min(root.width, root.height) * 0.75
        height: width
        radius: width * 0.5
        x: (root.width * 0.18) + Math.sin(plasmaTimer.angle) * 110
        y: (root.height * 0.22) + Math.cos(plasmaTimer.angle * 0.9) * 80
        opacity: 0.14
        color: "#00F2FE"
        antialiasing: true

        transform: Scale {
            origin.x: tealNode.width * 0.5
            origin.y: tealNode.height * 0.5
            xScale: 1.15 + Math.sin(plasmaTimer.angle * 1.4) * 0.25
            yScale: 1.15 + Math.cos(plasmaTimer.angle * 1.2) * 0.25
        }
    }

    // 2. Electric Violet Energy Node (#8B5CF6)
    Rectangle {
        id: violetNode
        width: Math.min(root.width, root.height) * 0.85
        height: width
        radius: width * 0.5
        x: (root.width * 0.62) - Math.cos(plasmaTimer.angle * 0.8) * 130
        y: (root.height * 0.38) - Math.sin(plasmaTimer.angle * 0.8) * 90
        opacity: 0.12
        color: "#8B5CF6"
        antialiasing: true

        transform: Scale {
            origin.x: violetNode.width * 0.5
            origin.y: violetNode.height * 0.5
            xScale: 1.2 + Math.cos(plasmaTimer.angle) * 0.2
            yScale: 1.2 + Math.sin(plasmaTimer.angle) * 0.2
        }
    }

    // 3. Cyber-Magenta Energy Node (#FF007F)
    Rectangle {
        id: magentaNode
        width: Math.min(root.width, root.height) * 0.65
        height: width
        radius: width * 0.5
        x: (root.width * 0.45) + Math.sin(plasmaTimer.angle * 1.2) * 95
        y: (root.height * 0.65) + Math.cos(plasmaTimer.angle * 1.1) * 75
        opacity: 0.10
        color: "#FF007F"
        antialiasing: true

        transform: Scale {
            origin.x: magentaNode.width * 0.5
            origin.y: magentaNode.height * 0.5
            xScale: 1.1 + Math.sin(plasmaTimer.angle * 1.6) * 0.18
            yScale: 1.1 + Math.cos(plasmaTimer.angle * 1.6) * 0.18
        }
    }

    // -------------------------------------------------------------------------
    // Matte Titanium Subtle Micro-Grid Texture
    // -------------------------------------------------------------------------
    ShaderEffect {
        anchors.fill: parent
        opacity: 0.05
        fragmentShader: "
            #version 440
            layout(location = 0) in vec2 qt_TexCoord0;
            layout(location = 0) out vec4 fragColor;
            void main() {
                vec2 grid = fract(qt_TexCoord0 * 80.0);
                float line = step(0.96, grid.x) + step(0.96, grid.y);
                fragColor = vec4(vec3(0.58, 0.64, 0.72) * clamp(line, 0.0, 1.0), 1.0);
            }
        "
    }

    // Plasma Animation Master Timer (60 FPS synced)
    Timer {
        id: plasmaTimer
        interval: 16
        running: true
        repeat: true
        property real angle: 0.0

        onTriggered: {
            angle += 0.007;
            if (angle > 6.2831853) angle = 0.0;
        }
    }
}
