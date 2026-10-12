#include "SchedulerBackend.h"

#include <QDebug>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <cstdlib>

SchedulerBackend::SchedulerBackend(QObject *parent)
    : QObject(parent)
{
    detectSupportedSchedulers();
    refreshState();

    connect(&m_telemetryTimer, &QTimer::timeout, this, &SchedulerBackend::updateTelemetry);
    m_telemetryTimer.start(1000);
    updateTelemetry();
}

SchedulerBackend::~SchedulerBackend()
{
    stopScheduler();
}

void SchedulerBackend::detectSupportedSchedulers()
{
    // Check if kernel supports sched-ext (CONFIG_SCHED_CLASS_EXT)
    m_isSupported = QFile::exists(QStringLiteral("/sys/kernel/sched_ext/state")) ||
                    QFile::exists(QStringLiteral("/sys/kernel/sched_ext"));

    // Check which scx binaries exist on disk
    QStringList candidates = {
        QStringLiteral("scx_bpfland"),
        QStringLiteral("scx_lavd"),
        QStringLiteral("scx_rusty"),
        QStringLiteral("scx_central"),
        QStringLiteral("scx_flash"),
        QStringLiteral("scx_nest"),
        QStringLiteral("scx_simple")
    };

    m_availableSchedulers.clear();
    for (const auto &cand : candidates) {
        if (QFile::exists(QStringLiteral("/usr/bin/") + cand) ||
            QFile::exists(QStringLiteral("/usr/local/bin/") + cand)) {
            m_availableSchedulers.append(cand);
        }
    }

    if (m_availableSchedulers.isEmpty()) {
        // Populate standard supported profiles
        m_availableSchedulers = {
            QStringLiteral("scx_bpfland (Low Latency Desktop)"),
            QStringLiteral("scx_lavd (Gaming & Frame Pacing)"),
            QStringLiteral("scx_rusty (Multi-Core Throughput)"),
            QStringLiteral("scx_central (Energy Optimized)")
        };
    }

    emit schedulersChanged();
}

void SchedulerBackend::readKernelState()
{
    QFile stateFile(QStringLiteral("/sys/kernel/sched_ext/state"));
    if (stateFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString st = QString::fromUtf8(stateFile.readAll()).trimmed();
        m_isRunning = (st == QStringLiteral("enabled"));
    } else {
        m_isRunning = false;
    }

    QFile opsFile(QStringLiteral("/sys/kernel/sched_ext/root/ops_name"));
    if (opsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_activeScheduler = QString::fromUtf8(opsFile.readAll()).trimmed();
    } else if (!m_isRunning) {
        m_activeScheduler = QStringLiteral("Default (CFS / EEVDF)");
    }
}

void SchedulerBackend::readGovernorState()
{
    QFile govFile(QStringLiteral("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"));
    if (govFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_activeGovernor = QString::fromUtf8(govFile.readAll()).trimmed();
    } else {
        m_activeGovernor = QStringLiteral("schedutil");
    }

    QFile availGov(QStringLiteral("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors"));
    if (availGov.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_availableGovernors = QString::fromUtf8(availGov.readAll()).trimmed().split(' ', Qt::SkipEmptyParts);
    } else {
        m_availableGovernors = { QStringLiteral("schedutil"), QStringLiteral("performance"), QStringLiteral("powersave") };
    }

    emit governorChanged();
}

void SchedulerBackend::refreshState()
{
    readKernelState();
    readGovernorState();
    emit statusChanged();
}

void SchedulerBackend::startScheduler(const QString &schedulerName)
{
    stopScheduler();

    QString binName = schedulerName.section(' ', 0, 0); // e.g. "scx_lavd"

    m_statusMessage = QStringLiteral("Activating kernel scheduler: %1...").arg(binName);
    emit statusChanged();

    m_schedulerProcess = std::make_unique<QProcess>();
    QString cmd = QStringLiteral("scxctl switch ") + binName;

    // Check if systemd service or direct binary is available
    if (QFile::exists(QStringLiteral("/usr/bin/") + binName) || QFile::exists(QStringLiteral("/usr/local/bin/") + binName)) {
        cmd = binName;
    }

    m_schedulerProcess->start(QStringLiteral("pkexec"), { QStringLiteral("/bin/bash"), QStringLiteral("-c"), cmd });

    m_isRunning = true;
    m_activeScheduler = binName;
    m_statusMessage = QStringLiteral("Kernel scheduler %1 active").arg(binName);
    emit statusChanged();
}

void SchedulerBackend::stopScheduler()
{
    if (m_schedulerProcess && m_schedulerProcess->state() != QProcess::NotRunning) {
        m_schedulerProcess->terminate();
        m_schedulerProcess->waitForFinished(1000);
        m_schedulerProcess->kill();
    }
    m_schedulerProcess.reset();

    QProcess::execute(QStringLiteral("pkexec"), { QStringLiteral("systemctl"), QStringLiteral("stop"), QStringLiteral("scx") });

    m_isRunning = false;
    m_activeScheduler = QStringLiteral("Default (CFS / EEVDF)");
    m_statusMessage = QStringLiteral("Kernel scheduler stopped; standard scheduler active.");
    emit statusChanged();
}

void SchedulerBackend::setGovernor(const QString &governor)
{
    if (governor.isEmpty()) return;

    QString cmd = QStringLiteral("echo ") + governor + QStringLiteral(" | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor");
    QProcess::execute(QStringLiteral("pkexec"), { QStringLiteral("/bin/bash"), QStringLiteral("-c"), cmd });

    m_activeGovernor = governor;
    m_statusMessage = QStringLiteral("CPU governor set to: %1").arg(governor);
    emit governorChanged();
    emit statusChanged();
}

void SchedulerBackend::updateTelemetry()
{
    // Read CPU Frequency
    QFile freqFile(QStringLiteral("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"));
    if (freqFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qreal khz = freqFile.readAll().trimmed().toDouble();
        m_cpuFreqGhz = khz / 1000000.0;
    } else {
        // Fallback to /proc/cpuinfo
        QFile cpuInfo(QStringLiteral("/proc/cpuinfo"));
        if (cpuInfo.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&cpuInfo);
            while (!stream.atEnd()) {
                QString line = stream.readLine();
                if (line.startsWith(QStringLiteral("cpu MHz"))) {
                    double mhz = line.section(':', 1, 1).trimmed().toDouble();
                    m_cpuFreqGhz = mhz / 1000.0;
                    break;
                }
            }
        }
    }

    // Read CPU Usage from /proc/stat
    QFile statFile(QStringLiteral("/proc/stat"));
    if (statFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString line = statFile.readLine();
        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.size() >= 5) {
            unsigned long long user = parts[1].toULongLong();
            unsigned long long nice = parts[2].toULongLong();
            unsigned long long system = parts[3].toULongLong();
            unsigned long long idle = parts[4].toULongLong();

            unsigned long long total = user + nice + system + idle;
            unsigned long long totalDiff = total - m_prevTotal;
            unsigned long long idleDiff = idle - m_prevIdle;

            if (totalDiff > 0) {
                m_cpuUsagePercent = static_cast<int>(100.0 * (1.0 - (double)idleDiff / totalDiff));
                m_cpuUsagePercent = qBound(0, m_cpuUsagePercent, 100);
            }

            m_prevTotal = total;
            m_prevIdle = idle;
        }
    }

    emit telemetryChanged();
}
