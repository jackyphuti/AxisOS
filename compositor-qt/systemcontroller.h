#pragma once

#include <QObject>
#include <QString>
#include <QProcess>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusMessage>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QFileSystemWatcher>
#include <memory>

class SystemController : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Notifications")

    // Theming & Design Tokens
    Q_PROPERTY(bool isDarkMode READ isDarkMode WRITE setDarkMode NOTIFY themeChanged)
    Q_PROPERTY(QString activeTheme READ activeTheme NOTIFY themeChanged)

    // Hardware Audio & Display Controls
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool isMuted READ isMuted WRITE setMuted NOTIFY volumeChanged)
    Q_PROPERTY(int brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)

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
    Q_PROPERTY(QString neuralPulseTarget READ neuralPulseTarget NOTIFY neuralPulseChanged)
    Q_PROPERTY(int activeNodeLinks READ activeNodeLinks NOTIFY nodeLinksChanged)

public:
    explicit SystemController(QObject *parent = nullptr);
    ~SystemController() override = default;

    // Getters
    [[nodiscard]] bool isDarkMode() const noexcept { return m_isDarkMode; }
    [[nodiscard]] QString activeTheme() const { return m_activeTheme; }
    [[nodiscard]] int volume() const noexcept { return m_volume; }
    [[nodiscard]] bool isMuted() const noexcept { return m_isMuted; }
    [[nodiscard]] int brightness() const noexcept { return m_brightness; }

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
    [[nodiscard]] QString neuralPulseTarget() const { return m_neuralPulseTarget; }
    [[nodiscard]] int activeNodeLinks() const noexcept { return m_activeNodeLinks; }

    // Q_INVOKABLE Actions
    Q_INVOKABLE void launchApp(const QString &command);
    Q_INVOKABLE QVariantList getInstalledApps();
    Q_INVOKABLE void toggleWifi(bool enabled);
    Q_INVOKABLE void toggleBluetooth(bool enabled);
    Q_INVOKABLE void powerOff();
    Q_INVOKABLE void reboot();
    Q_INVOKABLE void suspend();
    Q_INVOKABLE void shutdownSystem() { powerOff(); }
    Q_INVOKABLE void rebootSystem() { reboot(); }
    Q_INVOKABLE void suspendSystem() { suspend(); }
    Q_INVOKABLE void boostFocusedWindow(const QString &title, bool isFullscreen = false);
    Q_INVOKABLE void checkpointWindow(const QString &title);
    Q_INVOKABLE void rewindWindow(const QString &title);
    Q_INVOKABLE void connectLiquidNode(const QString &sourceTitle, const QString &targetTitle);

    // Theme & OSD controls
    Q_INVOKABLE void setDarkMode(bool dark);
    Q_INVOKABLE void toggleDarkMode();
    Q_INVOKABLE void setVolume(int vol);
    Q_INVOKABLE void setMuted(bool muted);
    Q_INVOKABLE void setBrightness(int b);
    Q_INVOKABLE void triggerOsd(const QString &type, int value, const QString &icon);

public slots:
    // org.freedesktop.Notifications Specification Implementation
    uint Notify(const QString &app_name, uint replaces_id, const QString &app_icon,
                const QString &summary, const QString &body, const QStringList &actions,
                const QVariantMap &hints, int expire_timeout);
    void CloseNotification(uint id);
    QStringList GetCapabilities();
    void GetServerInformation(QString &name, QString &vendor, QString &version, QString &spec_version);

signals:
    void themeChanged();
    void volumeChanged();
    void brightnessChanged();
    void osdTriggered(const QString &type, int value, const QString &icon);
    void notificationReceived(uint id, const QString &appName, const QString &summary,
                              const QString &body, const QString &appIcon, int timeout);
    void NotificationClosed(uint id, uint reason);
    void ActionInvoked(uint id, const QString &actionKey);

    void wifiChanged();
    void bluetoothChanged();
    void batteryChanged();
    void telemetryChanged();
    void neuralPulseChanged();
    void nodeLinksChanged();

private slots:
    void pollTelemetry();
    void updateNetworkManager();
    void updateUPower();
    void updateBlueZ();
    void onThemeConfigFileChanged(const QString &path);

private:
    void initDBusSubscribers();
    void initNotificationDaemon();
    void initThemeConfig();
    void readKernelVersion();

    // D-Bus interfaces
    std::unique_ptr<QDBusInterface> m_nmInterface;
    std::unique_ptr<QDBusInterface> m_upowerInterface;
    std::unique_ptr<QDBusInterface> m_bluezInterface;
    std::unique_ptr<QDBusInterface> m_loginInterface;

    // States
    bool m_isDarkMode{false};
    QString m_activeTheme{"AxisOneUI-Light"};
    int m_volume{65};
    bool m_isMuted{false};
    int m_brightness{80};

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
    QString m_neuralPulseTarget{"Desktop"};
    int m_activeNodeLinks{0};

    uint m_notificationCounter{1};

    // CPU telemetry tracking
    unsigned long long m_prevIdle{0};
    unsigned long long m_prevTotal{0};

    std::unique_ptr<QTimer> m_pollTimer;
    std::unique_ptr<QFileSystemWatcher> m_themeWatcher;
    QString m_themeConfigPath;
};
