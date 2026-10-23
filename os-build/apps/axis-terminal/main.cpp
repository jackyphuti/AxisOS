#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QIcon>
#include <QUrl>
#include <QStringList>
#include <QDebug>

#include "TerminalBackend.h"
#include "TerminalScreenItem.h"

int main(int argc, char *argv[])
{
    if (qEnvironmentVariableIsEmpty("QSG_RHI_PREFER_SOFTWARE_RENDERER")) {
        qputenv("QSG_RHI_PREFER_SOFTWARE_RENDERER", "0");
    }
    qputenv("QSG_INFO", "1");

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("axis-terminal"));
    app.setOrganizationName(QStringLiteral("AxisOS"));
    app.setApplicationVersion(QStringLiteral("2.1.0"));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("utilities-terminal")));

    // Parse optional -e / --command flags passed by launchers or scripts
    QString startupCommand;
    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) {
        if ((args[i] == QStringLiteral("-e") || args[i] == QStringLiteral("--command") || args[i] == QStringLiteral("-c")) && i + 1 < args.size()) {
            startupCommand = args.mid(i + 1).join(QLatin1Char(' '));
            break;
        }
    }

    QQuickWindow::setDefaultAlphaBuffer(true);

    qmlRegisterType<TerminalScreenItem>("AxisTerminal", 1, 0, "TerminalScreenItem");
    qmlRegisterType<TerminalBackend>("AxisTerminal", 1, 0, "TerminalBackend");

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("StartupCommand"), startupCommand);

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
