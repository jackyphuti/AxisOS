#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QTimer>
#include <QProcess>
#include <memory>

class SchedulerBackend : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isSupported READ isSupported NOTIFY statusChanged)
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY statusChanged)
    Q_PROPERTY(QString activeScheduler READ activeScheduler NOTIFY statusChanged)
    Q_PROPERTY(QString activeGovernor READ activeGovernor NOTIFY governorChanged)
    Q_PROPERTY(QStringList availableSchedulers READ availableSchedulers NOTIFY schedulersChanged)
    Q_PROPERTY(QStringList availableGovernors READ availableGovernors NOTIFY governorChanged)
    Q_PROPERTY(qreal cpuFreqGhz READ cpuFreqGhz NOTIFY telemetryChanged)
    Q_PROPERTY(int cpuUsagePercent READ cpuUsagePercent NOTIFY telemetryChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)

public:
    explicit SchedulerBackend(QObject *parent = nullptr);
    ~SchedulerBackend() override;

    bool isSupported() const { return m_isSupported; }
    bool isRunning() const { return m_isRunning; }
    QString activeScheduler() const { return m_activeScheduler; }
    QString activeGovernor() const { return m_activeGovernor; }
    QStringList availableSchedulers() const { return m_availableSchedulers; }
    QStringList availableGovernors() const { return m_availableGovernors; }
    qreal cpuFreqGhz() const { return m_cpuFreqGhz; }
    int cpuUsagePercent() const { return m_cpuUsagePercent; }
    QString statusMessage() const { return m_statusMessage; }

    Q_INVOKABLE void startScheduler(const QString &schedulerName);
    Q_INVOKABLE void stopScheduler();
    Q_INVOKABLE void setGovernor(const QString &governor);
    Q_INVOKABLE void refreshState();

Q_SIGNALS:
    void statusChanged();
    void governorChanged();
    void schedulersChanged();
    void telemetryChanged();

private Q_SLOTS:
    void updateTelemetry();

private:
    void detectSupportedSchedulers();
    void readKernelState();
    void readGovernorState();

    bool m_isSupported{false};
    bool m_isRunning{false};
    QString m_activeScheduler{QStringLiteral("Default (CFS / EEVDF)")};
    QString m_activeGovernor{QStringLiteral("schedutil")};
    QStringList m_availableSchedulers;
    QStringList m_availableGovernors;
    qreal m_cpuFreqGhz{2.8};
    int m_cpuUsagePercent{12};
    QString m_statusMessage;

    QTimer m_telemetryTimer;
    std::unique_ptr<QProcess> m_schedulerProcess;

    unsigned long long m_prevIdle{0};
    unsigned long long m_prevTotal{0};
};
