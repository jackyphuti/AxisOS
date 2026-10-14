#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QIcon>
#include <QUrl>
#include <QDebug>

#include "TerminalBackend.h"
#include "TerminalScreenItem.h"

int main(int argc, char *argv[])
{
    // Automatically detect and support hardware acceleration, with graceful
    // fallback to software rasterizer in VirtualBox / QEMU / VMs.
    if (qEnvironmentVariableIsEmpty("QSG_RHI_PREFER_SOFTWARE_RENDERER")) {
        // Allow Qt Quick to fall back to software renderer if GPU initialization fails
        qputenv("QSG_RHI_PREFER_SOFTWARE_RENDERER", "0");
    }
    qputenv("QSG_INFO", "1");

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("axis-terminal"));
    app.setOrganizationName(QStringLiteral("AxisOS"));
    app.setApplicationVersion(QStringLiteral("2.0.0"));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("utilities-terminal")));

    // Enable transparent alpha buffer for frosted glassmorphism
    QQuickWindow::setDefaultAlphaBuffer(true);

    // Register native C++ QQuickItem & Backend with QML engine
    qmlRegisterType<TerminalScreenItem>("AxisTerminal", 1, 0, "TerminalScreenItem");
    qmlRegisterType<TerminalBackend>("AxisTerminal", 1, 0, "TerminalBackend");

    QQmlApplicationEngine engine;

    const QUrl url(u"qrc:/AxisTerminal/TerminalWindow.qml"_qs);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            qCritical() << "[AxisTerminal] Failed to instantiate TerminalWindow.qml";
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);

    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        engine.load(QUrl::fromLocalFile(QStringLiteral("TerminalWindow.qml")));
    }

    return app.exec();
}
