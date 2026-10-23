pragma Singleton
import QtQuick

QtObject {
    id: theme

    // Live binding to SystemController.isDarkMode when running inside compositor, defaulting to Dark Mode (true)
    readonly property bool isDark: (typeof SystemController !== "undefined" && SystemController !== null && SystemController.isDarkMode !== undefined)
                                   ? SystemController.isDarkMode : true

    // Unified AxisOS One UI Dark & Light Color Tokens
    readonly property color base: isDark ? "#141826" : "#F4F6FB"
    readonly property color surface: isDark ? "#1F2231" : "#FFFFFF"
    readonly property color surfaceAlt: isDark ? "#262A3D" : "#F4F6FB"
    readonly property color surfaceElevated: isDark ? "#282C40" : "#FFFFFF"
    readonly property color textPrimary: isDark ? "#F2F3F7" : "#1C1C1E"
    readonly property color textSecondary: isDark ? "#9EA3B5" : "#6B6F7B"
    readonly property color accent: isDark ? "#7D95FF" : "#1428A0"
    readonly property color accentSoft: isDark ? "#2A3366" : "#E6EBFF"
    readonly property color divider: isDark ? "#2E3349" : "#E5E7EE"
    readonly property color success: "#2BB673"
    readonly property color warning: "#F59E0B"
    readonly property color danger: isDark ? "#F43F5E" : "#E11D48"

    // Glassmorphic Legibility & Specular Guards
    readonly property color glassTint: isDark ? "#381F2231" : "#38FFFFFF"
    readonly property color specularBorder: isDark ? "#26FFFFFF" : "#1A1428A0"

    // Radii (One UI design system)
    readonly property real radiusSm: 12
    readonly property real radiusMd: 16
    readonly property real radiusLg: 24
    readonly property real radiusPill: 999

    // Elevation shadows
    readonly property color shadowColor: isDark ? "#66000000" : "#141428A0"

    // Spring physics configuration
    readonly property real springConstant: 3.8
    readonly property real springDamping: 0.24
    readonly property real springEpsilon: 0.01
}
