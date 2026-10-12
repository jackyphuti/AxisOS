#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QIcon>
#include <QUrl>
#include <QDebug>

#include "SchedulerBackend.h"

int main(int argc, char *argv[])
{
    qputenv("QSG_RHI_PREFER_SOFTWARE_RENDERER", "0");

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("axis-scheduler-manager"));
    app.setOrganizationName(QStringLiteral("AxisOS"));
    app.setApplicationVersion(QStringLiteral("2.0.0"));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("preferences-system-performance")));

    QQuickWindow::setDefaultAlphaBuffer(true);

    auto backend = std::make_unique<SchedulerBackend>();

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("SchedulerBackend"), backend.get());

    const QUrl url(u"qrc:/AxisSchedulerManager/SchedulerWindow.qml"_qs);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            qCritical() << "[AxisSchedulerManager] Failed to load SchedulerWindow.qml";
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);

    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        engine.load(QUrl::fromLocalFile(QStringLiteral("SchedulerWindow.qml")));
    }

    return app.exec();
}
