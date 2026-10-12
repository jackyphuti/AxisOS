#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QIcon>
#include <QUrl>
#include <QDebug>

#include "WelcomeBackend.h"

int main(int argc, char *argv[])
{
    qputenv("QSG_RHI_PREFER_SOFTWARE_RENDERER", "0");

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("axis-welcome"));
    app.setOrganizationName(QStringLiteral("AxisOS"));
    app.setApplicationVersion(QStringLiteral("2.0.0"));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("axisos"), QIcon(QStringLiteral("/usr/share/pixmaps/axisos.png"))));

    QQuickWindow::setDefaultAlphaBuffer(true);

    auto backend = std::make_unique<WelcomeBackend>();

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("WelcomeBackend"), backend.get());

    const QUrl url(u"qrc:/AxisWelcome/WelcomeWindow.qml"_qs);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            qCritical() << "[AxisWelcome] Failed to load WelcomeWindow.qml";
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);

    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        engine.load(QUrl::fromLocalFile(QStringLiteral("WelcomeWindow.qml")));
    }

    return app.exec();
}
