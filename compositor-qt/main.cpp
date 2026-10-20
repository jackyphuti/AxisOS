#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>
#include <QDebug>
#include <QFileInfo>
#include <QQuickWindow>
#include <memory>
#include <cstdlib>

#include "systemcontroller.h"
#include "authmanager.h"

int main(int argc, char *argv[])
{
    // Configure high-performance GPU pipeline (Vulkan/OpenGL RHI)
    qputenv("QT_QPA_PLATFORM", "wayland;xcb;eglfs");
    qputenv("QSG_INFO", "1");
    qputenv("QSG_RHI_PREFER_SOFTWARE_RENDERER", "0");

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
