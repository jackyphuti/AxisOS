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

    // Background base layer: Deep Obsidian with subtle neon vignette
    Rectangle {
        anchors.fill: parent
        color: "#06070A"

        // Ambient radial cyber-glow (cyber-teal & electric-violet)
        Rectangle {
            width: parent.width * 0.7
            height: parent.height * 0.7
            anchors.centerIn: parent
            radius: width / 2
            opacity: 0.12
            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop { position: 0.0; color: "#00F2FE" }
                GradientStop { position: 0.6; color: "#7928CA" }
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
        color: "#CC121622" // Frosted obsidian glass

        // Elastic jelly animation on card height as biometric items expand/collapse
        Behavior on implicitHeight {
            NumberAnimation { duration: 320; easing.type: Easing.OutBack; easing.overshoot: 1.1 }
        }

        // Cyber-Teal / Electric-Violet Glassmorphic Border Highlight
        border.width: 1
        border.color: AuthManager.authError ? "#80FF007A" : (passwordInput.activeFocus ? "#6600F2FE" : "#26334D")

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
                        border.color: AuthManager.isAuthenticating ? "#00F2FE" : "#2EC4B6"
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
                        color: "#18212D"

                        // User profile glyph (SVG vector canvas)
                        Canvas {
                            anchors.centerIn: parent
                            width: 32
                            height: 32
                            onPaint: {
                                var ctx = getContext("2d");
                                ctx.reset();
                                ctx.fillStyle = "#E6EDE8";
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
                    color: "#F8FAFC"
                }

                // Username & Session Subtitle
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: AuthManager.username + "@axis-station"
                    font.pixelSize: 12
                    font.letterSpacing: 0.5
                    color: "#94A3B8"
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
                            color: "#00F2FE"
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
                            color: "#151C28"
                            border.width: 1.5
                            border.color: AuthManager.isFingerprintActive ? "#00F2FE" : "#2EC4B6"

                            // Hardware-rendered Cyber-Teal Fingerprint Vector
                            Canvas {
                                id: fpCanvas
                                anchors.centerIn: parent
                                width: 34
                                height: 34
                                onPaint: {
                                    var ctx = getContext("2d");
                                    ctx.reset();
                                    ctx.strokeStyle = AuthManager.isFingerprintActive ? "#00F2FE" : "#2EC4B6";
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
                        color: AuthManager.isFingerprintActive ? "#00F2FE" : "#8A9A92"
                    }
                }

                // --- B. Facial Recognition (Howdy / IR) Indicator ---
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 8
                    visible: AuthManager.hasFaceAuth
                    Layout.preferredHeight: visible ? implicitHeight : 0

                    // Rotating Cyber-Cyan Scanning Iris
                    Item {
                        width: 22
                        height: 22

                        Rectangle {
                            id: irisScan
                            anchors.fill: parent
                            radius: 11
                            color: "transparent"
                            border.width: 2
                            border.color: "#2EC4B6"

                            RotationAnimation on rotation {
                                running: AuthManager.isFaceAuthActive
                                loops: Animation.Infinite
                                from: 0; to: 360; duration: 1200
                            }
                        }
                    }

                    Text {
                        text: AuthManager.isFaceAuthActive ? "Looking for face..." : "Face recognition ready"
                        font.pixelSize: 12
                        color: AuthManager.isFaceAuthActive ? "#2EC4B6" : "#8A9A92"
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
                    height: 48
                    radius: 14
                    color: "#18211D"
                    border.width: 1.5
                    border.color: passwordInput.activeFocus ? "#00F2FE" : (AuthManager.authError ? "#FF007A" : "#2A3644")

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
                            color: "#F8FAFC"
                            font.pixelSize: 14
                            focus: true
                            clip: true

                            Text {
                                anchors.fill: parent
                                text: "Enter Password or PIN..."
                                color: "#64748B"
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
                            color: passwordInput.text.length > 0 ? "#00F2FE" : "#223029"
                            border.width: 1
                            border.color: passwordInput.text.length > 0 ? "#00F2FE" : "#2EC4B6"
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
                                    ctx.strokeStyle = passwordInput.text.length > 0 ? "#06070A" : "#8A9A92";
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

                // Authentication Status / Error Message
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: AuthManager.authMessage
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    color: AuthManager.authError ? "#FF007A" : (AuthManager.isAuthenticating ? "#00F2FE" : "#2EC4B6")
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
            color: "#18211D"
            border.width: 1
            border.color: "#2EC4B6"
            opacity: 0.8

            Canvas {
                anchors.centerIn: parent
                width: 18
                height: 18
                onPaint: {
                    var ctx = getContext("2d");
                    ctx.reset();
                    ctx.strokeStyle = "#E6EDE8";
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
            color: "#18211D"
            border.width: 1
            border.color: "#2EC4B6"
            opacity: 0.8

            Canvas {
                anchors.centerIn: parent
                width: 18
                height: 18
                onPaint: {
                    var ctx = getContext("2d");
                    ctx.reset();
                    ctx.strokeStyle = "#E6EDE8";
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
            color: "#18211D"
            border.width: 1
            border.color: "#FF007A"
            opacity: 0.8

            Canvas {
                anchors.centerIn: parent
                width: 18
                height: 18
                onPaint: {
                    var ctx = getContext("2d");
                    ctx.reset();
                    ctx.strokeStyle = "#FF007A";
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
