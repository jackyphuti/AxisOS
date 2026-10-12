#include "WelcomeBackend.h"

#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QStandardPaths>
#include <QThread>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

#include <sys/utsname.h>

WelcomeBackend::WelcomeBackend(QObject *parent)
    : QObject(parent)
{
    m_cpuCores = QThread::idealThreadCount();
    m_isLiveSession = QFile::exists(QStringLiteral("/run/live")) || 
                      qEnvironmentVariable("USER") == QStringLiteral("axisos") ||
                      qEnvironmentVariable("USER") == QStringLiteral("live");

    detectHardware();
    readMemoryInfo();
    checkStartupStatus();
    checkGameMode();

    connect(&m_telemetryTimer, &QTimer::timeout, this, &WelcomeBackend::updateTelemetry);
    m_telemetryTimer.start(2500);
}

void WelcomeBackend::detectHardware()
{
    struct utsname unameData;
    if (uname(&unameData) == 0) {
        m_kernelVersion = QString::fromUtf8(unameData.release);
    }

    QFile cpuFile(QStringLiteral("/proc/cpuinfo"));
    if (cpuFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&cpuFile);
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            if (line.startsWith(QStringLiteral("model name"))) {
                int colonIdx = line.indexOf(':');
                if (colonIdx != -1) {
                    m_cpuModel = line.mid(colonIdx + 1).trimmed();
                    break;
                }
            }
        }
    }

    // Identify GPU via DRM sysfs
    QDir drmDir(QStringLiteral("/sys/class/drm"));
    QStringList cardDirs = drmDir.entryList(QStringList() << QStringLiteral("card*"), QDir::Dirs | QDir::NoDotAndDotDot);
    QString foundGpu;
    for (const auto &card : cardDirs) {
        if (!card.contains('-')) {
            QString devicePath = QStringLiteral("/sys/class/drm/%1/device").arg(card);
            QFile ueventFile(devicePath + QStringLiteral("/uevent"));
            if (ueventFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream ustream(&ueventFile);
                while (!ustream.atEnd()) {
                    QString uline = ustream.readLine();
                    if (uline.startsWith(QStringLiteral("DRIVER="))) {
                        QString drv = uline.mid(7).trimmed();
                        if (drv == QStringLiteral("nvidia") || drv == QStringLiteral("nouveau")) {
                            foundGpu = QStringLiteral("NVIDIA Dedicated Graphics (%1)").arg(drv);
                        } else if (drv == QStringLiteral("amdgpu") || drv == QStringLiteral("radeon")) {
                            foundGpu = QStringLiteral("AMD Radeon GPU (%1)").arg(drv);
                        } else if (drv == QStringLiteral("i915") || drv == QStringLiteral("xe")) {
                            foundGpu = QStringLiteral("Intel HD / Arc Graphics (%1)").arg(drv);
                        }
                    }
                }
            }
        }
    }

    if (foundGpu.isEmpty()) {
        QProcess lspciProc;
        lspciProc.start(QStringLiteral("lspci"), QStringList() << QStringLiteral("-mm"));
        if (lspciProc.waitForFinished(1000)) {
            QString out = QString::fromUtf8(lspciProc.readAllStandardOutput());
            for (const QString &line : out.split('\n')) {
                if (line.contains(QStringLiteral("VGA compatible controller")) || line.contains(QStringLiteral("3D controller"))) {
                    QStringList parts = line.split('"');
                    if (parts.size() >= 6) {
                        foundGpu = parts[3] + QStringLiteral(" ") + parts[5];
                        break;
                    }
                }
            }
        }
    }

    if (!foundGpu.isEmpty()) {
        m_gpuModel = foundGpu;
    }
}

void WelcomeBackend::readMemoryInfo()
{
    unsigned long long memTotalKb = 0;
    unsigned long long memAvailKb = 0;

    QFile memFile(QStringLiteral("/proc/meminfo"));
    if (memFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&memFile);
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            if (line.startsWith(QStringLiteral("MemTotal:"))) {
                memTotalKb = line.split(QLatin1Char(' '), Qt::SkipEmptyParts).value(1).toULongLong();
            } else if (line.startsWith(QStringLiteral("MemAvailable:"))) {
                memAvailKb = line.split(QLatin1Char(' '), Qt::SkipEmptyParts).value(1).toULongLong();
            }
        }
    }

    if (memTotalKb > 0) {
        m_ramTotalGb = static_cast<qreal>(memTotalKb) / (1024.0 * 1024.0);
        unsigned long long memUsedKb = (memTotalKb >= memAvailKb) ? (memTotalKb - memAvailKb) : 0;
        m_ramUsedGb = static_cast<qreal>(memUsedKb) / (1024.0 * 1024.0);
        m_ramPercent = static_cast<int>((m_ramUsedGb / m_ramTotalGb) * 100.0);
    }

    // Read zRAM status from /proc/swaps
    unsigned long long zramTotalKb = 0;
    unsigned long long zramUsedKb = 0;
    bool hasZram = false;

    QFile swapsFile(QStringLiteral("/proc/swaps"));
    if (swapsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&swapsFile);
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            if (line.contains(QStringLiteral("zram"))) {
                hasZram = true;
                QStringList parts = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
                if (parts.size() >= 4) {
                    zramTotalKb += parts.value(2).toULongLong();
                    zramUsedKb += parts.value(3).toULongLong();
                }
            }
        }
    }

    m_zramActive = hasZram;
    if (hasZram && zramTotalKb > 0) {
        m_zramTotalGb = static_cast<qreal>(zramTotalKb) / (1024.0 * 1024.0);
        m_zramUsedGb = static_cast<qreal>(zramUsedKb) / (1024.0 * 1024.0);
    }
}

void WelcomeBackend::checkStartupStatus()
{
    QString autostartDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/autostart");
    QString desktopPath = autostartDir + QStringLiteral("/axis-welcome.desktop");
    m_showOnStartup = QFile::exists(desktopPath);
}

void WelcomeBackend::checkGameMode()
{
    QProcess p;
    p.start(QStringLiteral("gamemoded"), QStringList() << QStringLiteral("-s"));
    if (p.waitForFinished(500)) {
        QString out = QString::fromUtf8(p.readAllStandardOutput());
        m_gameModeActive = out.contains(QStringLiteral("is active"));
    }
}

void WelcomeBackend::updateTelemetry()
{
    readMemoryInfo();
    Q_EMIT telemetryChanged();
}

void WelcomeBackend::refreshTelemetry()
{
    detectHardware();
    readMemoryInfo();
    checkGameMode();
    Q_EMIT telemetryChanged();
    Q_EMIT gameModeChanged();
}

void WelcomeBackend::launchApp(const QString &appId)
{
    if (appId == QStringLiteral("scheduler")) {
        QProcess::startDetached(QStringLiteral("/usr/local/bin/axis-scheduler-manager"), {});
    } else if (appId == QStringLiteral("installer")) {
        QProcess::startDetached(QStringLiteral("/usr/local/bin/axis-package-installer"), {});
    } else if (appId == QStringLiteral("settings")) {
        QProcess::startDetached(QStringLiteral("/usr/local/bin/axis-settings"), {});
    } else if (appId == QStringLiteral("terminal")) {
        QProcess::startDetached(QStringLiteral("/usr/local/bin/axis-terminal"), {});
    } else if (appId == QStringLiteral("os-installer")) {
        QProcess::startDetached(QStringLiteral("/usr/local/bin/axis-installer"), {});
    } else if (appId == QStringLiteral("nvidia")) {
        QProcess::startDetached(QStringLiteral("/usr/local/bin/axis-terminal"), 
            QStringList() << QStringLiteral("-e") << QStringLiteral("pkexec /usr/local/bin/nvidia-auto-install.sh"));
    } else if (appId == QStringLiteral("browser")) {
        if (QFile::exists(QStringLiteral("/usr/bin/firefox"))) {
            QProcess::startDetached(QStringLiteral("/usr/bin/firefox"), {});
        } else if (QFile::exists(QStringLiteral("/usr/bin/librewolf"))) {
            QProcess::startDetached(QStringLiteral("/usr/bin/librewolf"), {});
        } else {
            QProcess::startDetached(QStringLiteral("x-www-browser"), {});
        }
    }
}

void WelcomeBackend::toggleStartup(bool enable)
{
    QString autostartDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/autostart");
    QDir().mkpath(autostartDir);
    QString desktopPath = autostartDir + QStringLiteral("/axis-welcome.desktop");

    if (enable) {
        QFile file(desktopPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "[Desktop Entry]\n"
                << "Type=Application\n"
                << "Name=AxisOS Welcome\n"
                << "Exec=/usr/local/bin/axis-welcome\n"
                << "Icon=preferences-desktop\n"
                << "Terminal=false\n"
                << "StartupNotify=false\n";
        }
        m_showOnStartup = true;
    } else {
        QFile::remove(desktopPath);
        m_showOnStartup = false;
    }
    Q_EMIT startupChanged();
}

void WelcomeBackend::toggleGameMode()
{
    if (m_gameModeActive) {
        QProcess::execute(QStringLiteral("gamemoded"), QStringList() << QStringLiteral("-t"));
        m_gameModeActive = false;
    } else {
        QProcess::execute(QStringLiteral("gamemoded"), QStringList() << QStringLiteral("-r"));
        m_gameModeActive = true;
    }
    Q_EMIT gameModeChanged();
}

void WelcomeBackend::optimizeZram()
{
    // Apply XanMod recommended zRAM kernel sysctl
    QProcess::startDetached(QStringLiteral("pkexec"), 
        QStringList() << QStringLiteral("sysctl") << QStringLiteral("-w") << QStringLiteral("vm.swappiness=180")
                      << QStringLiteral("vm.watermark_boost_factor=0")
                      << QStringLiteral("vm.watermark_scale_factor=125")
                      << QStringLiteral("vm.page-cluster=0"));
    updateTelemetry();
}

void WelcomeBackend::openDocumentation()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/jackympoka/AxisOS")));
}
