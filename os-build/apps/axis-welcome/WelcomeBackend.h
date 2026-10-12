#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QProcess>

class WelcomeBackend : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString osName READ osName CONSTANT)
    Q_PROPERTY(QString osVersion READ osVersion CONSTANT)
    Q_PROPERTY(QString kernelVersion READ kernelVersion NOTIFY telemetryChanged)
    Q_PROPERTY(QString cpuModel READ cpuModel NOTIFY telemetryChanged)
    Q_PROPERTY(int cpuCores READ cpuCores CONSTANT)
    Q_PROPERTY(qreal ramTotalGb READ ramTotalGb NOTIFY telemetryChanged)
    Q_PROPERTY(qreal ramUsedGb READ ramUsedGb NOTIFY telemetryChanged)
    Q_PROPERTY(int ramPercent READ ramPercent NOTIFY telemetryChanged)
    Q_PROPERTY(qreal zramTotalGb READ zramTotalGb NOTIFY telemetryChanged)
    Q_PROPERTY(qreal zramUsedGb READ zramUsedGb NOTIFY telemetryChanged)
    Q_PROPERTY(bool zramActive READ zramActive NOTIFY telemetryChanged)
    Q_PROPERTY(QString gpuModel READ gpuModel NOTIFY telemetryChanged)
    Q_PROPERTY(QString displayServer READ displayServer CONSTANT)
    Q_PROPERTY(bool gameModeActive READ gameModeActive NOTIFY gameModeChanged)
    Q_PROPERTY(bool showOnStartup READ showOnStartup NOTIFY startupChanged)
    Q_PROPERTY(bool isLiveSession READ isLiveSession CONSTANT)

public:
    explicit WelcomeBackend(QObject *parent = nullptr);
    ~WelcomeBackend() override = default;

    QString osName() const { return QStringLiteral("AxisOS"); }
    QString osVersion() const { return QStringLiteral("2.0.0 (Liquid Nodes)"); }
    QString kernelVersion() const { return m_kernelVersion; }
    QString cpuModel() const { return m_cpuModel; }
    int cpuCores() const { return m_cpuCores; }
    qreal ramTotalGb() const { return m_ramTotalGb; }
    qreal ramUsedGb() const { return m_ramUsedGb; }
    int ramPercent() const { return m_ramPercent; }
    qreal zramTotalGb() const { return m_zramTotalGb; }
    qreal zramUsedGb() const { return m_zramUsedGb; }
    bool zramActive() const { return m_zramActive; }
    QString gpuModel() const { return m_gpuModel; }
    QString displayServer() const { return QStringLiteral("Qt 6 Wayland (LiquidNodesOS)"); }
    bool gameModeActive() const { return m_gameModeActive; }
    bool showOnStartup() const { return m_showOnStartup; }
    bool isLiveSession() const { return m_isLiveSession; }

    Q_INVOKABLE void launchApp(const QString &appId);
    Q_INVOKABLE void toggleStartup(bool enable);
    Q_INVOKABLE void toggleGameMode();
    Q_INVOKABLE void optimizeZram();
    Q_INVOKABLE void refreshTelemetry();
    Q_INVOKABLE void openDocumentation();

Q_SIGNALS:
    void telemetryChanged();
    void gameModeChanged();
    void startupChanged();

private Q_SLOTS:
    void updateTelemetry();

private:
    void detectHardware();
    void readMemoryInfo();
    void checkStartupStatus();
    void checkGameMode();

    QString m_kernelVersion{QStringLiteral("Linux XanMod")};
    QString m_cpuModel{QStringLiteral("x86_64 Processor")};
    int m_cpuCores{4};
    qreal m_ramTotalGb{16.0};
    qreal m_ramUsedGb{2.4};
    int m_ramPercent{15};
    qreal m_zramTotalGb{8.0};
    qreal m_zramUsedGb{0.2};
    bool m_zramActive{true};
    QString m_gpuModel{QStringLiteral("Hardware Accelerated GPU")};
    bool m_gameModeActive{false};
    bool m_showOnStartup{true};
    bool m_isLiveSession{false};

    QTimer m_telemetryTimer;
};
