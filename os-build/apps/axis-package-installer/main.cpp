#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QIcon>
#include <QUrl>
#include <QDebug>

#include "PackageBackend.h"

int main(int argc, char *argv[])
{
    qputenv("QSG_RHI_PREFER_SOFTWARE_RENDERER", "0");

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("axis-package-installer"));
    app.setOrganizationName(QStringLiteral("AxisOS"));
    app.setApplicationVersion(QStringLiteral("2.0.0"));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("system-software-install")));

    QQuickWindow::setDefaultAlphaBuffer(true);

    auto backend = std::make_unique<PackageBackend>();

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("PackageBackend"), backend.get());

    const QUrl url(u"qrc:/AxisPackageInstaller/PackageInstallerWindow.qml"_qs);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            qCritical() << "[AxisPackageInstaller] Failed to load PackageInstallerWindow.qml";
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);

    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        engine.load(QUrl::fromLocalFile(QStringLiteral("PackageInstallerWindow.qml")));
    }

    return app.exec();
}
