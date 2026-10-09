#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>
#include <QDebug>
#include <QFileInfo>

int main(int argc, char *argv[])
{
    // Enable high-DPI scaling and hardware graphics backend
    qputenv("QT_QPA_PLATFORM", "wayland;xcb;eglfs");
    qputenv("QSG_INFO", "1");

    QGuiApplication app(argc, argv);
    app.setApplicationName("LiquidNodesOS");
    app.setOrganizationName("AxisOS");
    app.setApplicationVersion("2.0.0");

    QQmlApplicationEngine engine;

    const QUrl qrcUrl(u"qrc:/LiquidOS/Main.qml"_qs);
    const QUrl localUrl = QUrl::fromLocalFile(QStringLiteral("/usr/share/axis-compositor-qt/Main.qml"));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [qrcUrl](QObject *obj, const QUrl &objUrl) {
        if (!obj && objUrl == qrcUrl) {
            qCritical() << "Failed to load Main.qml from resources";
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);

    engine.load(qrcUrl);

    if (engine.rootObjects().isEmpty()) {
        qWarning() << "Resource URL failed, falling back to local file...";
        engine.load(QUrl(QStringLiteral("Main.qml")));
    }

    return app.exec();
}
