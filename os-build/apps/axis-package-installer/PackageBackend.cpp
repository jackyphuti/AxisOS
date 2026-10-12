#include "PackageBackend.h"

#include <QDebug>
#include <QDir>
#include <QFile>

PackageBackend::PackageBackend(QObject *parent)
    : QObject(parent)
{
    initPackageData();
}

PackageBackend::~PackageBackend()
{
    cancelInstallation();
}

void PackageBackend::initPackageData()
{
    m_categories = {
        QStringLiteral("Browsers"),
        QStringLiteral("Gaming"),
        QStringLiteral("Development"),
        QStringLiteral("Media"),
        QStringLiteral("System"),
        QStringLiteral("Office")
    };

    m_packages = {
        // Browsers
        { "firefox", "Firefox", "Fast, private web browser from Mozilla", "firefox", "Browsers", false },
        { "chromium", "Chromium", "Open-source web browser engine behind Chrome", "chromium", "Browsers", false },
        { "brave-browser", "Brave", "Privacy-focused browser with ad-blocking built in", "brave-browser", "Browsers", false },
        { "torbrowser-launcher", "Tor Browser", "Anonymity and censorship evasion network browser", "tor-browser", "Browsers", false },

        // Gaming
        { "steam", "Steam", "Valve's premier digital gaming distribution platform", "steam", "Gaming", false },
        { "lutris", "Lutris", "Open gaming platform for Windows, Epic, and GOG titles", "lutris", "Gaming", false },
        { "heroic", "Heroic Games", "Native launcher for Epic Games and GOG", "heroic", "Gaming", false },
        { "gamemode", "GameMode", "Daemon optimizing system performance on demand", "preferences-system", "Gaming", false },
        { "mangohud", "MangoHud", "Vulkan and OpenGL overlay for monitoring FPS and temps", "utilities-system-monitor", "Gaming", false },
        { "wine", "Wine", "Compatibility layer capable of running Windows apps", "wine", "Gaming", false },

        // Development
        { "code", "VS Code", "Extensible code editor from Microsoft", "visual-studio-code", "Development", false },
        { "git", "Git", "Fast, distributed version control system", "git", "Development", false },
        { "build-essential", "Build Essential", "GCC, G++, Make, and standard C/C++ development tools", "applications-development", "Development", false },
        { "python3-pip", "Python 3 & Pip", "Python 3 environment and package manager", "python", "Development", false },
        { "nodejs", "Node.js & NPM", "Asynchronous event-driven JavaScript runtime", "nodejs", "Development", false },
        { "clang", "LLVM Clang", "C/C++/Objective-C compiler toolchain", "applications-development", "Development", false },

        // Media
        { "vlc", "VLC Media Player", "Versatile multimedia player for virtually all audio/video formats", "vlc", "Media", false },
        { "obs-studio", "OBS Studio", "Free and open source software for video recording and live streaming", "obs", "Media", false },
        { "audacity", "Audacity", "Multi-track audio editor and recorder", "audacity", "Media", false },
        { "kdenlive", "Kdenlive", "Powerful non-linear video editing suite", "kdenlive", "Media", false },
        { "gimp", "GIMP", "GNU Image Manipulation Program for photo retouching", "gimp", "Media", false },

        // System
        { "gparted", "GParted", "Graphical partition editor for disks and drives", "gparted", "System", false },
        { "htop", "Htop", "Interactive process viewer and system monitor", "utilities-terminal", "System", false },
        { "fastfetch", "Fastfetch", "Neofetch-like tool for fetching system information quickly", "utilities-terminal", "System", false },
        { "zram-tools", "zRAM Tools", "Compressed RAM swap block device manager", "preferences-system", "System", false },
        { "btrfs-progs", "Btrfs Utilities", "File system utilities for BTRFS volumes and snapshots", "drive-harddisk", "System", false },

        // Office
        { "libreoffice", "LibreOffice", "Comprehensive office productivity suite", "libreoffice-main", "Office", false },
        { "thunderbird", "Thunderbird", "Email, calendar, and contacts client", "thunderbird", "Office", false },
        { "evince", "Document Viewer", "Lightweight document viewer for PDF and PostScript", "x-office-document", "Office", false }
    };

    // Initial check of which packages are installed
    for (auto &pkg : m_packages) {
        pkg.installed = isPackageInstalled(pkg.id);
    }
}

bool PackageBackend::isPackageInstalled(const QString &pkgName)
{
    QProcess check;
    check.start(QStringLiteral("dpkg-query"), { QStringLiteral("-W"), QStringLiteral("-f=${Status}"), pkgName });
    check.waitForFinished(1500);
    QString output = QString::fromUtf8(check.readAllStandardOutput());
    return output.contains(QStringLiteral("install ok installed"));
}

QVariantList PackageBackend::getCategories()
{
    QVariantList list;
    for (const auto &cat : m_categories) {
        list.append(cat);
    }
    return list;
}

QVariantList PackageBackend::getPackagesForCategory(const QString &category)
{
    QVariantList list;
    for (const auto &pkg : m_packages) {
        if (pkg.category == category) {
            QVariantMap map;
            map[QStringLiteral("id")] = pkg.id;
            map[QStringLiteral("name")] = pkg.name;
            map[QStringLiteral("description")] = pkg.description;
            map[QStringLiteral("icon")] = pkg.icon;
            map[QStringLiteral("category")] = pkg.category;
            map[QStringLiteral("installed")] = pkg.installed;
            map[QStringLiteral("selected")] = m_selectedPackages.contains(pkg.id);
            list.append(map);
        }
    }
    return list;
}

bool PackageBackend::isPackageSelected(const QString &pkgName) const
{
    return m_selectedPackages.contains(pkgName);
}

void PackageBackend::togglePackageSelection(const QString &pkgName)
{
    if (m_selectedPackages.contains(pkgName)) {
        m_selectedPackages.removeAll(pkgName);
    } else {
        m_selectedPackages.append(pkgName);
    }
    emit selectionChanged();
}

void PackageBackend::clearSelection()
{
    m_selectedPackages.clear();
    emit selectionChanged();
}

void PackageBackend::installSelected()
{
    if (m_isBusy || m_selectedPackages.isEmpty()) {
        return;
    }

    m_isBusy = true;
    m_statusMessage = QStringLiteral("Installing %1 package(s)...").arg(m_selectedPackages.size());
    m_log.clear();
    emit busyChanged();
    emit statusChanged();
    emit logChanged();

    QStringList installArgs;
    installArgs << QStringLiteral("DEBIAN_FRONTEND=noninteractive")
                << QStringLiteral("apt-get")
                << QStringLiteral("install")
                << QStringLiteral("-y")
                << QStringLiteral("--no-install-recommends");
    installArgs.append(m_selectedPackages);

    QString fullCmd = installArgs.join(' ');

    m_process = std::make_unique<QProcess>();
    connect(m_process.get(), &QProcess::readyReadStandardOutput, this, &PackageBackend::onProcessOutput);
    connect(m_process.get(), &QProcess::readyReadStandardError, this, &PackageBackend::onProcessOutput);
    connect(m_process.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &PackageBackend::onProcessFinished);

    if (getuid() == 0) {
        m_process->start(QStringLiteral("/bin/bash"), { QStringLiteral("-c"), fullCmd });
    } else {
        m_process->start(QStringLiteral("pkexec"), { QStringLiteral("/bin/bash"), QStringLiteral("-c"), fullCmd });
    }
}

void PackageBackend::cancelInstallation()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        m_process->waitForFinished(1000);
        m_process->kill();
    }
    m_isBusy = false;
    emit busyChanged();
}

void PackageBackend::onProcessOutput()
{
    if (!m_process) return;

    QByteArray data = m_process->readAllStandardOutput() + m_process->readAllStandardError();
    m_log.append(QString::fromUtf8(data));
    emit logChanged();
}

void PackageBackend::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_isBusy = false;
    bool success = (exitCode == 0 && exitStatus == QProcess::NormalExit);

    if (success) {
        m_statusMessage = QStringLiteral("Installation completed successfully!");
        for (const auto &id : m_selectedPackages) {
            for (auto &pkg : m_packages) {
                if (pkg.id == id) pkg.installed = true;
            }
        }
        m_selectedPackages.clear();
    } else {
        m_statusMessage = QStringLiteral("Installation encountered an error (exit code: %1)").arg(exitCode);
    }

    emit busyChanged();
    emit statusChanged();
    emit selectionChanged();
    emit installFinished(success);
}
