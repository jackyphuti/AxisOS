import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: loginRoot
    anchors.fill: parent
    visible: AuthManager.isLocked
    z: 9999

    // Track shake offset for authentication errors
    property real shakeOffset: 0
    property bool showPasswordOverride: false

    // Background base layer: Deep Obsidian with subtle neon vignette
    Rectangle {
        anchors.fill: parent
        color: Theme.base

        // Ambient radial cyber-glow (cyber-teal & electric-violet)
        Rectangle {
            width: parent.width * 0.7
            height: parent.height * 0.7
            anchors.centerIn: parent
            radius: width / 2
            opacity: 0.12
            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop { position: 0.0; color: Theme.accent }
                GradientStop { position: 0.6; color: Theme.accentSoft }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }
    }

    // Shake animation for login card on authentication failure
    SequentialAnimation {
        id: shakeAnim
        running: false
        NumberAnimation { target: loginRoot; property: "shakeOffset"; to: -14; duration: 50; easing.type: Easing.OutQuad }
        NumberAnimation { target: loginRoot; property: "shakeOffset"; to: 14; duration: 50; easing.type: Easing.InOutQuad }
        NumberAnimation { target: loginRoot; property: "shakeOffset"; to: -10; duration: 50; easing.type: Easing.InOutQuad }
        NumberAnimation { target: loginRoot; property: "shakeOffset"; to: 10; duration: 50; easing.type: Easing.InOutQuad }
        NumberAnimation { target: loginRoot; property: "shakeOffset"; to: -5; duration: 40; easing.type: Easing.InOutQuad }
        NumberAnimation { target: loginRoot; property: "shakeOffset"; to: 5; duration: 40; easing.type: Easing.InOutQuad }
        NumberAnimation { target: loginRoot; property: "shakeOffset"; to: 0; duration: 50; easing.type: Easing.OutBack }
    }

    Connections {
        target: AuthManager
        function onAuthFailed(errorMessage) {
            shakeAnim.restart();
            passwordInput.selectAll();
            passwordInput.forceActiveFocus();
        }
        function onAuthenticated() {
            passwordInput.text = "";
        }
    }

    // -------------------------------------------------------------------------
    // Frosted Glassmorphic Login Prism Card
    // -------------------------------------------------------------------------
    Rectangle {
        id: loginCard
        width: 440
        implicitHeight: cardLayout.implicitHeight + 64
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: loginRoot.shakeOffset
        radius: 24
        color: Theme.surface

        // Elastic jelly animation on card height as biometric items expand/collapse
        Behavior on implicitHeight {
            NumberAnimation { duration: 320; easing.type: Easing.OutBack; easing.overshoot: 1.1 }
        }

        // Cyber-Teal / Electric-Violet Glassmorphic Border Highlight
        border.width: 1
        border.color: AuthManager.authError ? Theme.danger : (passwordInput.activeFocus ? Theme.accent : Theme.divider)

        Behavior on border.color {
            ColorAnimation { duration: 250 }
        }

        ColumnLayout {
            id: cardLayout
            anchors.fill: parent
            anchors.margins: 32
            spacing: 20

            // 1. User Profile Section
            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 10

                // Avatar Container
                Item {
                    Layout.alignment: Qt.AlignHCenter
                    width: 76
                    height: 76

                    // Outer neon cyber-teal ring
                    Rectangle {
                        anchors.fill: parent
                        radius: 38
                        color: "transparent"
                        border.width: 2
                        border.color: AuthManager.isAuthenticating ? Theme.accent : Theme.divider
                        opacity: 0.85

                        // Pulsing ring when authenticating
                        SequentialAnimation on scale {
                            running: AuthManager.isAuthenticating
                            loops: Animation.Infinite
                            NumberAnimation { to: 1.06; duration: 700; easing.type: Easing.InOutQuad }
                            NumberAnimation { to: 1.0; duration: 700; easing.type: Easing.InOutQuad }
                        }
                    }

                    // Avatar Inner Circle
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 4
                        radius: 34
                        color: Theme.surfaceAlt

                        // User profile glyph (SVG vector canvas)
                        Canvas {
                            anchors.centerIn: parent
                            width: 32
                            height: 32
                            onPaint: {
                                var ctx = getContext("2d");
                                ctx.reset();
                                ctx.fillStyle = Theme.textPrimary.toString();
                                // Head
                                ctx.beginPath();
                                ctx.arc(16, 11, 7, 0, 2 * Math.PI);
                                ctx.fill();
                                // Torso
                                ctx.beginPath();
                                ctx.moveTo(6, 29);
                                ctx.quadraticCurveTo(6, 21, 16, 21);
                                ctx.quadraticCurveTo(26, 21, 26, 29);
                                ctx.closePath();
                                ctx.fill();
                            }
                        }
                    }
                }

                // Display Name
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: AuthManager.displayName
                    font.pixelSize: 22
                    font.weight: Font.DemiBold
                    color: Theme.textPrimary
                }

                // Username & Session Subtitle
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: AuthManager.username + "@axis-station"
                    font.pixelSize: 12
                    font.letterSpacing: 0.5
                    color: Theme.textSecondary
                }
            }

            // -----------------------------------------------------------------
            // 2. Dynamic Biometrics Section (Collapses smoothly if no hardware)
            // -----------------------------------------------------------------
            ColumnLayout {
                id: biometricsContainer
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                spacing: 12
                visible: AuthManager.hasFingerprint || AuthManager.hasFaceAuth

                // Dynamic height collapse: when invisible, height is 0
                Layout.preferredHeight: visible ? implicitHeight : 0
                opacity: visible ? 1.0 : 0.0

                Behavior on opacity {
                    NumberAnimation { duration: 250 }
                }

                // --- A. Fingerprint Scanner Indicator ---
                ColumnLayout {
                    id: fingerprintSection
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 8
                    visible: AuthManager.hasFingerprint
                    Layout.preferredHeight: visible ? implicitHeight : 0

                    Item {
                        Layout.alignment: Qt.AlignHCenter
                        width: 68
                        height: 68

                        // Glowing radial pulse backplane
                        Rectangle {
                            id: fpPulseGlow
                            anchors.fill: parent
                            radius: 34
                            color: Theme.accent
                            opacity: 0.15
                            scale: 1.0

                            // Sequential Pulse Loop on fingerprint prompt
                            SequentialAnimation {
                                id: fpPulseAnim
                                running: AuthManager.isFingerprintActive
                                loops: Animation.Infinite
                                ParallelAnimation {
                                    NumberAnimation { target: fpPulseGlow; property: "scale"; to: 1.25; duration: 600; easing.type: Easing.OutQuad }
                                    NumberAnimation { target: fpPulseGlow; property: "opacity"; to: 0.45; duration: 600; easing.type: Easing.OutQuad }
                                }
                                ParallelAnimation {
                                    NumberAnimation { target: fpPulseGlow; property: "scale"; to: 1.0; duration: 600; easing.type: Easing.InOutQuad }
                                    NumberAnimation { target: fpPulseGlow; property: "opacity"; to: 0.15; duration: 600; easing.type: Easing.InOutQuad }
                                }
                            }
                        }

                        // Fingerprint Housing Circle
                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 4
                            radius: 30
                            color: Theme.surfaceAlt
                            border.width: 1.5
                            border.color: AuthManager.isFingerprintActive ? Theme.accent : Theme.divider

                            // Hardware-rendered Cyber-Teal Fingerprint Vector
                            Canvas {
                                id: fpCanvas
                                anchors.centerIn: parent
                                width: 34
                                height: 34
                                onPaint: {
                                    var ctx = getContext("2d");
                                    ctx.reset();
                                    ctx.strokeStyle = (AuthManager.isFingerprintActive ? Theme.accent : Theme.textSecondary).toString();
                                    ctx.lineWidth = 2.0;
                                    ctx.lineCap = "round";

                                    // Outer ridge
                                    ctx.beginPath();
                                    ctx.arc(17, 17, 13, Math.PI * 0.75, Math.PI * 2.25);
                                    ctx.stroke();

                                    // Middle ridge
                                    ctx.beginPath();
                                    ctx.arc(17, 17, 9, Math.PI * 0.65, Math.PI * 2.15);
                                    ctx.stroke();

                                    // Inner ridge
                                    ctx.beginPath();
                                    ctx.arc(17, 17, 5, Math.PI * 0.5, Math.PI * 1.9);
                                    ctx.stroke();

                                    // Core loop
                                    ctx.beginPath();
                                    ctx.moveTo(17, 15);
                                    ctx.lineTo(17, 24);
                                    ctx.stroke();
                                }

                                Connections {
                                    target: AuthManager
                                    function onFingerprintStateChanged() { fpCanvas.requestPaint(); }
                                    function onFingerprintPulseRequested() { fpCanvas.requestPaint(); }
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                AuthManager.startBiometricAuth();
                            }
                        }
                    }

                    // Fingerprint status text
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: AuthManager.isFingerprintActive ? "Touch fingerprint reader" : "Fingerprint scan ready"
                        font.pixelSize: 12
                        font.weight: Font.Medium
                        color: AuthManager.isFingerprintActive ? Theme.accent : Theme.textSecondary
                    }
                }

                // --- B. Infrared Iris & Facial Scanner Indicator ---
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 8
                    visible: AuthManager.hasFaceAuth
                    Layout.preferredHeight: visible ? implicitHeight : 0

                    // Rotating Cyber-Cyan Scanning Iris
                    Item {
                        width: 26
                        height: 26

                        Rectangle {
                            id: irisScan
                            anchors.fill: parent
                            radius: 13
                            color: Theme.surfaceAlt
                            border.width: 2
                            border.color: AuthManager.isFaceAuthActive ? Theme.accent : Theme.divider

                            Rectangle {
                                anchors.centerIn: parent
                                width: 10
                                height: 10
                                radius: 5
                                color: AuthManager.isFaceAuthActive ? Theme.accent : Theme.textSecondary
                            }

                            RotationAnimation on rotation {
                                running: AuthManager.isFaceAuthActive
                                loops: Animation.Infinite
                                from: 0; to: 360; duration: 1200
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: AuthManager.startIrisAuth()
                        }
                    }

                    Text {
                        text: AuthManager.isFaceAuthActive ? "Scanning iris via IR sensor..." : "Iris / IR scanner ready (click to scan)"
                        font.pixelSize: 12
                        color: AuthManager.isFaceAuthActive ? Theme.accent : Theme.textSecondary
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: AuthManager.startIrisAuth()
                        }
                    }
                }

                // --- C. Passwordless Quick Unlock Button (when LOCK_AUTH_MODE=none) ---
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true
                    height: 44
                    radius: 14
                    visible: AuthManager.passwordlessUnlock
                    color: Theme.accent
                    Text {
                        anchors.centerIn: parent
                        text: "Click to Unlock Session (No Password Required)"
                        color: Theme.base
                        font.pixelSize: 13
                        font.weight: Font.Bold
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: AuthManager.unlockSession()
                    }
                }
            }

            // -----------------------------------------------------------------
            // 3. Password / PIN Input Field (Dynamically centered)
            // -----------------------------------------------------------------
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Rectangle {
                    id: inputContainer
                    Layout.fillWidth: true
                    visible: !AuthManager.passwordlessUnlock && (loginRoot.showPasswordOverride || (AuthManager.lockAuthMode !== "fingerprint_only" && AuthManager.lockAuthMode !== "iris_only"))
                    height: 48
                    radius: 14
                    color: Theme.surfaceAlt
                    border.width: 1.5
                    border.color: passwordInput.activeFocus ? Theme.accent : (AuthManager.authError ? Theme.danger : Theme.divider)

                    Behavior on border.color {
                        ColorAnimation { duration: 200 }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 8
                        spacing: 10

                        // Password Input
                        TextInput {
                            id: passwordInput
                            Layout.fillWidth: true
                            echoMode: TextInput.Password
                            color: Theme.textPrimary
                            font.pixelSize: 14
                            focus: true
                            clip: true

                            Text {
                                anchors.fill: parent
                                text: "Enter Password or PIN..."
                                color: Theme.textSecondary
                                font.pixelSize: 14
                                visible: !passwordInput.text && !passwordInput.inputMethodComposing
                            }

                            onAccepted: {
                                if (passwordInput.text.length > 0) {
                                    AuthManager.authenticate(passwordInput.text);
                                }
                            }
                        }

                        // Unlock / Action Button
                        Rectangle {
                            width: 34
                            height: 34
                            radius: 17
                            color: passwordInput.text.length > 0 ? Theme.accent : Theme.surfaceElevated
                            border.width: 1
                            border.color: passwordInput.text.length > 0 ? Theme.accent : Theme.divider
                            opacity: AuthManager.isAuthenticating ? 0.6 : 1.0

                            Behavior on color {
                                ColorAnimation { duration: 180 }
                            }

                            Canvas {
                                anchors.centerIn: parent
                                width: 14
                                height: 14
                                onPaint: {
                                    var ctx = getContext("2d");
                                    ctx.reset();
                                    ctx.strokeStyle = (passwordInput.text.length > 0 ? "#FFFFFF" : Theme.textSecondary).toString();
                                    ctx.lineWidth = 2.0;
                                    ctx.lineCap = "round";
                                    ctx.lineJoin = "round";
                                    // Right arrow glyph
                                    ctx.beginPath();
                                    ctx.moveTo(2, 7);
                                    ctx.lineTo(12, 7);
                                    ctx.moveTo(7, 2);
                                    ctx.lineTo(12, 7);
                                    ctx.lineTo(7, 12);
                                    ctx.stroke();
                                }
                                Connections {
                                    target: passwordInput
                                    function onTextChanged() { parent.children[0].requestPaint(); }
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (passwordInput.text.length > 0) {
                                        AuthManager.authenticate(passwordInput.text);
                                    }
                                }
                            }
                        }
                    }
                }

                // Biometric-only fallback link to show password field if needed
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    visible: !AuthManager.passwordlessUnlock && !inputContainer.visible
                    text: "Use Password or PIN instead"
                    font.pixelSize: 11
                    color: Theme.textSecondary
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: loginRoot.showPasswordOverride = true
                    }
                }

                // Authentication Status / Error Message
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: AuthManager.authMessage
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    color: AuthManager.authError ? Theme.danger : (AuthManager.isAuthenticating ? Theme.accent : Theme.success)
                    visible: text.length > 0
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // Bottom Session & Power Controls
    // -------------------------------------------------------------------------
    RowLayout {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 32
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 24

        // Sleep / Suspend Button
        Rectangle {
            width: 44
            height: 44
            radius: 22
            color: Theme.surfaceAlt
            border.width: 1
            border.color: Theme.divider
            opacity: 0.8

            Canvas {
                anchors.centerIn: parent
                width: 18
                height: 18
                onPaint: {
                    var ctx = getContext("2d");
                    ctx.reset();
                    ctx.strokeStyle = Theme.textPrimary.toString();
                    ctx.lineWidth = 1.8;
                    // Crescent moon (sleep)
                    ctx.beginPath();
                    ctx.arc(9, 9, 7, 0.5 * Math.PI, 1.8 * Math.PI);
                    ctx.quadraticCurveTo(8, 9, 13, 4);
                    ctx.stroke();
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    SystemController.suspendSystem();
                }
            }
        }

        // Reboot Button
        Rectangle {
            width: 44
            height: 44
            radius: 22
            color: Theme.surfaceAlt
            border.width: 1
            border.color: Theme.divider
            opacity: 0.8

            Canvas {
                anchors.centerIn: parent
                width: 18
                height: 18
                onPaint: {
                    var ctx = getContext("2d");
                    ctx.reset();
                    ctx.strokeStyle = Theme.textPrimary.toString();
                    ctx.lineWidth = 1.8;
                    // Refresh circular arrow
                    ctx.beginPath();
                    ctx.arc(9, 9, 6, 0.2 * Math.PI, 1.6 * Math.PI);
                    ctx.stroke();
                    ctx.beginPath();
                    ctx.moveTo(13, 3);
                    ctx.lineTo(15, 6);
                    ctx.lineTo(12, 7);
                    ctx.stroke();
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    SystemController.rebootSystem();
                }
            }
        }

        // Power Off Button
        Rectangle {
            width: 44
            height: 44
            radius: 22
            color: Theme.surfaceAlt
            border.width: 1
            border.color: Theme.danger
            opacity: 0.8

            Canvas {
                anchors.centerIn: parent
                width: 18
                height: 18
                onPaint: {
                    var ctx = getContext("2d");
                    ctx.reset();
                    ctx.strokeStyle = Theme.danger.toString();
                    ctx.lineWidth = 1.8;
                    // Power icon
                    ctx.beginPath();
                    ctx.arc(9, 9, 6, -0.3 * Math.PI, 1.3 * Math.PI);
                    ctx.stroke();
                    ctx.beginPath();
                    ctx.moveTo(9, 2);
                    ctx.lineTo(9, 8);
                    ctx.stroke();
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    SystemController.shutdownSystem();
                }
            }
        }
    }
}
