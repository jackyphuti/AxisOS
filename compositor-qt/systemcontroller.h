#pragma once

#include <QObject>
#include <QString>
#include <QProcess>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusMessage>
#include <QTimer>
#include <memory>

class SystemController : public QObject {
    Q_OBJECT

    // Connectivity & Hardware
    Q_PROPERTY(bool wifiConnected READ wifiConnected NOTIFY wifiChanged)
    Q_PROPERTY(QString wifiSsid READ wifiSsid NOTIFY wifiChanged)
    Q_PROPERTY(int wifiStrength READ wifiStrength NOTIFY wifiChanged)
    Q_PROPERTY(bool bluetoothEnabled READ bluetoothEnabled NOTIFY bluetoothChanged)
    Q_PROPERTY(int bluetoothConnectedDevices READ bluetoothConnectedDevices NOTIFY bluetoothChanged)

    // Power & Battery
    Q_PROPERTY(int batteryPercent READ batteryPercent NOTIFY batteryChanged)
    Q_PROPERTY(bool batteryCharging READ batteryCharging NOTIFY batteryChanged)
    Q_PROPERTY(bool hasBattery READ hasBattery NOTIFY batteryChanged)

    // Performance & Kernel Telemetry (XanMod + zRAM)
    Q_PROPERTY(int cpuLoadPercent READ cpuLoadPercent NOTIFY telemetryChanged)
    Q_PROPERTY(int ramUsagePercent READ ramUsagePercent NOTIFY telemetryChanged)
    Q_PROPERTY(int zramSavedMb READ zramSavedMb NOTIFY telemetryChanged)
    Q_PROPERTY(QString kernelVersion READ kernelVersion CONSTANT)

public:
    explicit SystemController(QObject *parent = nullptr);
    ~SystemController() override = default;

    // Getters
    [[nodiscard]] bool wifiConnected() const noexcept { return m_wifiConnected; }
    [[nodiscard]] QString wifiSsid() const { return m_wifiSsid; }
    [[nodiscard]] int wifiStrength() const noexcept { return m_wifiStrength; }
    [[nodiscard]] bool bluetoothEnabled() const noexcept { return m_bluetoothEnabled; }
    [[nodiscard]] int bluetoothConnectedDevices() const noexcept { return m_bluetoothConnectedDevices; }
    [[nodiscard]] int batteryPercent() const noexcept { return m_batteryPercent; }
    [[nodiscard]] bool batteryCharging() const noexcept { return m_batteryCharging; }
    [[nodiscard]] bool hasBattery() const noexcept { return m_hasBattery; }
    [[nodiscard]] int cpuLoadPercent() const noexcept { return m_cpuLoadPercent; }
    [[nodiscard]] int ramUsagePercent() const noexcept { return m_ramUsagePercent; }
    [[nodiscard]] int zramSavedMb() const noexcept { return m_zramSavedMb; }
    [[nodiscard]] QString kernelVersion() const { return m_kernelVersion; }

    // Q_INVOKABLE Actions
    Q_INVOKABLE void launchApp(const QString &command);
    Q_INVOKABLE void toggleWifi(bool enabled);
    Q_INVOKABLE void toggleBluetooth(bool enabled);
    Q_INVOKABLE void powerOff();
    Q_INVOKABLE void reboot();
    Q_INVOKABLE void suspend();

signals:
    void wifiChanged();
    void bluetoothChanged();
    void batteryChanged();
    void telemetryChanged();
    void notificationReceived(const QString &title, const QString &body);

private slots:
    void pollTelemetry();
    void updateNetworkManager();
    void updateUPower();
    void updateBlueZ();

private:
    void initDBusSubscribers();
    void readKernelVersion();

    // D-Bus interfaces
    std::unique_ptr<QDBusInterface> m_nmInterface;
    std::unique_ptr<QDBusInterface> m_upowerInterface;
    std::unique_ptr<QDBusInterface> m_bluezInterface;
    std::unique_ptr<QDBusInterface> m_loginInterface;

    // States
    bool m_wifiConnected{false};
    QString m_wifiSsid{"Offline"};
    int m_wifiStrength{0};
    bool m_bluetoothEnabled{false};
    int m_bluetoothConnectedDevices{0};
    int m_batteryPercent{100};
    bool m_batteryCharging{false};
    bool m_hasBattery{true};
    int m_cpuLoadPercent{0};
    int m_ramUsagePercent{0};
    int m_zramSavedMb{0};
    QString m_kernelVersion{"Linux XanMod"};

    // CPU telemetry tracking
    unsigned long long m_prevIdle{0};
    unsigned long long m_prevTotal{0};

    std::unique_ptr<QTimer> m_pollTimer;
};
