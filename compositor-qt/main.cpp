#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QQuickWindow>
#include <memory>
#include <cstdlib>

#include "systemcontroller.h"
#include "authmanager.h"

int main(int argc, char *argv[])
{
    // Setup XDG runtime directory with required 0700 permissions
    if (qEnvironmentVariableIsEmpty("XDG_RUNTIME_DIR")) {
        qputenv("XDG_RUNTIME_DIR", "/tmp/runtime-axis");
    }
    QString runtimeDir = QString::fromLocal8Bit(qgetenv("XDG_RUNTIME_DIR"));
    QDir().mkpath(runtimeDir);
    QFile::setPermissions(runtimeDir, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);

    // Dynamic QPA platform selection (avoid invalid semicolon syntax)
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        if (!qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY") &&
            QFile::exists(runtimeDir + QLatin1Char('/') + QString::fromLocal8Bit(qgetenv("WAYLAND_DISPLAY")))) {
            qputenv("QT_QPA_PLATFORM", "wayland");
        } else if (!qEnvironmentVariableIsEmpty("DISPLAY")) {
            qputenv("QT_QPA_PLATFORM", "xcb");
        } else if (QFile::exists(QStringLiteral("/dev/dri/card0")) || QFile::exists(QStringLiteral("/dev/dri/renderD128"))) {
            qputenv("QT_QPA_PLATFORM", "eglfs");
            if (qEnvironmentVariableIsEmpty("QT_QPA_EGLFS_INTEGRATION")) {
                qputenv("QT_QPA_EGLFS_INTEGRATION", "eglfs_kms");
            }
        } else {
            // Software frame buffer fallback for VMs without 3D DRM
            qputenv("QT_QPA_PLATFORM", "linuxfb");
        }
    }

    if (qEnvironmentVariableIsEmpty("QSG_INFO")) {
        qputenv("QSG_INFO", "1");
    }

    // Wayland Compositor server environment setup
    if (qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")) {
        qputenv("WAYLAND_DISPLAY", "wayland-0");
    }
    qputenv("XDG_CURRENT_DESKTOP", "AxisOS:LiquidNodes");
    qputenv("XDG_SESSION_TYPE", "wayland");

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("LiquidNodesOS"));
    app.setOrganizationName(QStringLiteral("AxisOS"));
    app.setApplicationVersion(QStringLiteral("2.0.0"));

    // Enable high-quality anti-aliasing and depth buffer
    QQuickWindow::setDefaultAlphaBuffer(true);

    QQmlApplicationEngine engine;

    // Instantiate native C++ D-Bus SystemController and expose directly to QML
    auto systemController = std::make_unique<SystemController>(&app);
    engine.rootContext()->setContextProperty(QStringLiteral("SystemController"), systemController.get());

    // Instantiate native C++ PAM/Biometrics AuthManager and expose directly to QML
    auto authManager = std::make_unique<AuthManager>(&app);
    engine.rootContext()->setContextProperty(QStringLiteral("AuthManager"), authManager.get());

    const QUrl qrcUrl(u"qrc:/LiquidNodes/Main.qml"_qs);
    const QUrl qrcUrl2(u"qrc:/qt/qml/LiquidNodes/Main.qml"_qs);
    const QUrl localUrl = QUrl::fromLocalFile(QStringLiteral("/usr/share/axis-compositor-qt/Main.qml"));

    engine.load(qrcUrl);
    if (engine.rootObjects().isEmpty()) {
        engine.load(qrcUrl2);
    }
    if (engine.rootObjects().isEmpty() && QFileInfo::exists(localUrl.toLocalFile())) {
        qInfo() << "[LiquidNodesOS] Loading from system QML directory:" << localUrl;
        engine.load(localUrl);
    }
    if (engine.rootObjects().isEmpty()) {
        qWarning() << "[LiquidNodesOS] Resource URL failed, falling back to working directory...";
        engine.load(QUrl(QStringLiteral("Main.qml")));
    }

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "[LiquidNodesOS] Fatal: Unable to load Main.qml from any location!";
        return -1;
    }

    return app.exec();
}
