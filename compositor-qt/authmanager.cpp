#include "authmanager.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <cstring>
#include <cstdlib>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <linux/videodev2.h>

AuthManager::AuthManager(QObject *parent)
    : QObject(parent)
{
    // Resolve logged-in username
    QString detectedUser = QString::fromUtf8(qgetenv("USER"));
    if (detectedUser.isEmpty()) {
        detectedUser = QString::fromUtf8(qgetenv("LOGNAME"));
    }
    if (detectedUser.isEmpty() || detectedUser == QStringLiteral("root")) {
        // Find default desktop user in /etc/passwd with UID 1000
        struct passwd *pw = getpwuid(1000);
        if (pw && pw->pw_name) {
            detectedUser = QString::fromUtf8(pw->pw_name);
        } else {
            detectedUser = QStringLiteral("axis");
        }
    }
    m_username = detectedUser;

    // Resolve user display name
    struct passwd *pw = getpwnam(m_username.toUtf8().constData());
    if (pw && pw->pw_gecos && strlen(pw->pw_gecos) > 0) {
        QString gecos = QString::fromUtf8(pw->pw_gecos);
        m_displayName = gecos.section(',', 0, 0).trimmed();
    }
    if (m_displayName.isEmpty()) {
        m_displayName = QStringLiteral("AxisOS User");
    }

    // Initial probe of biometric hardware
    checkHardware();

    // Check for Live ISO or automated autologin
    bool autologinRequested = qEnvironmentVariableIsSet("AXISOS_AUTOLOGIN")
                           || QFile::exists(QStringLiteral("/etc/axisos-autologin"))
                           || QFile::exists(QStringLiteral("/run/live"))
                           || QFile::exists(QStringLiteral("/etc/axisos-live"));
    if (!autologinRequested) {
        QFile cmdline(QStringLiteral("/proc/cmdline"));
        if (cmdline.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = QString::fromUtf8(cmdline.readAll());
            if (content.contains(QStringLiteral("autologin")) || content.contains(QStringLiteral("boot=live"))) {
                autologinRequested = true;
            }
            cmdline.close();
        }
    }

    if (autologinRequested) {
        m_isLocked = false;
        m_isAuthenticated = true;
        qInfo() << "[AuthManager] Autologin activated for user:" << m_username;
    } else {
        m_isLocked = true;
        m_isAuthenticated = false;
    }
}

AuthManager::~AuthManager()
{
    cancelAuthentication();
}

void AuthManager::setUsername(const QString &user)
{
    if (m_username != user && !user.isEmpty()) {
        m_username = user;
        struct passwd *pw = getpwnam(m_username.toUtf8().constData());
        if (pw && pw->pw_gecos && strlen(pw->pw_gecos) > 0) {
            QString gecos = QString::fromUtf8(pw->pw_gecos);
            m_displayName = gecos.section(',', 0, 0).trimmed();
        } else {
            m_displayName = m_username;
        }
        emit usernameChanged();
    }
}

void AuthManager::setIsLocked(bool locked)
{
    // Prevent unlocking if the session is locked and the user has not authenticated,
    // unless passwordless unlock is explicitly configured.
    if (!locked && m_isLocked && !m_isAuthenticated && !passwordlessUnlock()) {
        return;
    }

    if (m_isLocked != locked) {
        m_isLocked = locked;
        if (m_isLocked) {
            checkHardware();
            m_isAuthenticated = false;
            m_authError = false;
            m_authMessage.clear();
            emit authStateChanged();
            if (m_lockAuthMode == QStringLiteral("fingerprint_only") || m_lockAuthMode == QStringLiteral("iris_only")) {
                startBiometricAuth();
            }
        }
        emit lockStateChanged();
    }
}


void AuthManager::loadBiometricConfig()
{
    m_lockAuthMode = QStringLiteral("biometric_or_password");
    m_fpEnabledCfg = true;
    m_irisEnabledCfg = true;

    QStringList confPaths = {
        QStringLiteral("/etc/axisos/biometrics.conf"),
        QDir::homePath() + QStringLiteral("/.config/axis/biometrics.conf")
    };

    for (const QString &path : confPaths) {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            while (!f.atEnd()) {
                QString line = QString::fromUtf8(f.readLine()).trimmed();
                if (line.startsWith(QLatin1String("LOCK_AUTH_MODE="))) {
                    m_lockAuthMode = line.mid(15).trimmed();
                } else if (line.startsWith(QLatin1String("FINGERPRINT_ENABLED="))) {
                    m_fpEnabledCfg = (line.mid(20).trimmed().toLower() == QLatin1String("true"));
                } else if (line.startsWith(QLatin1String("IRIS_ENABLED="))) {
                    m_irisEnabledCfg = (line.mid(13).trimmed().toLower() == QLatin1String("true"));
                }
            }
            f.close();
        }
    }
}

void AuthManager::checkHardware()
{
    loadBiometricConfig();
    bool oldFp = m_hasFingerprint;
    bool oldFace = m_hasFaceAuth;

    bool hwFp = detectFingerprintHardware();
    bool hwIris = detectFaceAuthHardware();
    m_hasFingerprint = m_fpEnabledCfg && (hwFp || m_lockAuthMode == QStringLiteral("fingerprint_only"));
    m_hasIrisAuth = m_irisEnabledCfg && (hwIris || m_lockAuthMode == QStringLiteral("iris_only"));
    m_hasFaceAuth = m_hasIrisAuth;

    qInfo() << "[AuthManager] Biometric hardware detection:"
            << "Fingerprint scanner:" << (m_hasFingerprint ? "FOUND" : "NOT DETECTED")
            << "| Facial recognition:" << (m_hasFaceAuth ? "FOUND" : "NOT DETECTED");

    emit hardwareChanged();
}

bool AuthManager::detectFingerprintHardware()
{
    // 1. Query fprintd daemon over system D-Bus (net.reactivated.Fprint.Manager)
    QDBusInterface fprintd(
        QStringLiteral("net.reactivated.Fprint.Manager"),
        QStringLiteral("/net/reactivated/Fprint/Manager"),
        QStringLiteral("net.reactivated.Fprint.Manager"),
        QDBusConnection::systemBus()
    );

    if (fprintd.isValid()) {
        // Call GetDefaultDevice
        QDBusReply<QDBusObjectPath> reply = fprintd.call(QStringLiteral("GetDefaultDevice"));
        if (reply.isValid()) {
            const QString path = reply.value().path();
            if (!path.isEmpty() && path != QStringLiteral("/")) {
                qInfo() << "[AuthManager] Found active fprintd device at:" << path;
                return true;
            }
        } else {
            qDebug() << "[AuthManager] fprintd GetDefaultDevice reply:" << reply.error().name() << reply.error().message();
        }

        // Check GetDevices method as secondary check
        QDBusReply<QList<QDBusObjectPath>> listReply = fprintd.call(QStringLiteral("GetDevices"));
        if (listReply.isValid() && !listReply.value().isEmpty()) {
            qInfo() << "[AuthManager] Discovered" << listReply.value().size() << "fprint devices via GetDevices.";
            return true;
        }
    }

    // 2. Fallback check: Examine sysfs USB bus for known fingerprint vendor IDs
    QDir devDir(QStringLiteral("/sys/bus/usb/devices"));
    if (devDir.exists()) {
        const auto entries = devDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &entry : entries) {
            QFile idVendor(devDir.filePath(entry) + QStringLiteral("/idVendor"));
            if (idVendor.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString vendor = QString::fromUtf8(idVendor.readAll()).trimmed().toLower();
                // Known fingerprint scanner vendors: Goodix (27c6), Synaptics (06cb), Elan (04f3), Broadcom (0a5c), Validity (138a, 147e)
                if (vendor == "27c6" || vendor == "06cb" || vendor == "04f3" || vendor == "0a5c" || vendor == "138a" || vendor == "147e" || vendor == "05ba" || vendor == "1c7a" || vendor == "2808" || vendor == "10a5") {
                    qInfo() << "[AuthManager] Detected biometric USB vendor in sysfs:" << vendor;
                    return true;
                }
            }
        }
    }

    return false;
}

bool AuthManager::detectFaceAuthHardware()
{
    // 1. Check if Howdy or facial recognition software is installed
    bool howdyInstalled = QFile::exists(QStringLiteral("/usr/local/bin/axis-biometrics")) ||
                          QFile::exists(QStringLiteral("/var/lib/axisos/biometrics/iris/") + m_username + QStringLiteral(".iris")) ||
                          QFile::exists(QDir::homePath() + QStringLiteral("/.config/axis/") + m_username + QStringLiteral(".iris")) ||
                          QFile::exists(QStringLiteral("/usr/bin/howdy")) ||
                          QFile::exists(QStringLiteral("/etc/howdy/config.ini")) ||
                          QFile::exists(QStringLiteral("/lib/security/pam_howdy.so")) ||
                          QFile::exists(QStringLiteral("/usr/lib/security/pam_howdy.so"));

    // 2. Scan V4L2 video devices for infrared sensor capabilities
    QDir devDir(QStringLiteral("/dev"));
    const auto videoDevices = devDir.entryList(QStringList() << QStringLiteral("video*"), QDir::System);
    bool irSensorFound = false;

    for (const QString &devName : videoDevices) {
        QString devPath = QStringLiteral("/dev/") + devName;
        int fd = ::open(devPath.toUtf8().constData(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            continue;
        }

        struct v4l2_capability cap{};
        if (::ioctl(fd, VIDIOC_QUERYCAP, &cap) == 0) {
            QString cardName = QString::fromUtf8(reinterpret_cast<char*>(cap.card)).toLower();
            if (cardName.contains(QStringLiteral("ir")) ||
                cardName.contains(QStringLiteral("infra")) ||
                cardName.contains(QStringLiteral("realsense")) ||
                cardName.contains(QStringLiteral("azure"))) {
                irSensorFound = true;
                ::close(fd);
                break;
            }

            // Enumerate pixel formats looking for infrared/greyscale depth sensors
            struct v4l2_fmtdesc fmtdesc{};
            fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            while (::ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
                if (fmtdesc.pixelformat == V4L2_PIX_FMT_GREY ||
#ifdef V4L2_PIX_FMT_Y10P
                    fmtdesc.pixelformat == V4L2_PIX_FMT_Y10P ||
#endif
#ifdef V4L2_PIX_FMT_Y16
                    fmtdesc.pixelformat == V4L2_PIX_FMT_Y16 ||
#endif
#ifdef V4L2_PIX_FMT_Z16
                    fmtdesc.pixelformat == V4L2_PIX_FMT_Z16 ||
#endif
                    fmtdesc.pixelformat == v4l2_fourcc('Y', '1', '0', ' ') ||
                    fmtdesc.pixelformat == v4l2_fourcc('Y', '1', '2', ' ')) {
                    irSensorFound = true;
                    break;
                }
                fmtdesc.index++;
            }
        }
        ::close(fd);
        if (irSensorFound) {
            break;
        }
    }

    return (howdyInstalled && irSensorFound) || (irSensorFound && QFile::exists(QStringLiteral("/usr/bin/howdy")));
}

int AuthManager::pamConversationCallback(int num_msg,
                                        const struct pam_message **msg,
                                        struct pam_response **resp,
                                        void *appdata_ptr)
{
    if (num_msg <= 0 || !resp || !appdata_ptr) {
        return PAM_CONV_ERR;
    }

    auto *ctx = static_cast<PamWorkerContext *>(appdata_ptr);
    if (ctx->abortFlag && ctx->abortFlag->load()) {
        return PAM_ABORT;
    }

    auto *responses = static_cast<struct pam_response *>(
        calloc(num_msg, sizeof(struct pam_response)));
    if (!responses) {
        return PAM_BUF_ERR;
    }

    for (int i = 0; i < num_msg; ++i) {
        const struct pam_message *m = msg[i];
        if (!m) {
            continue;
        }

        QString promptText = QString::fromUtf8(m->msg ? m->msg : "");

        switch (m->msg_style) {
        case PAM_PROMPT_ECHO_OFF:
            // Secret prompt: user password
            responses[i].resp = strdup(ctx->password.c_str());
            responses[i].resp_retcode = 0;
            break;

        case PAM_PROMPT_ECHO_ON:
            // Plaintext prompt: username
            responses[i].resp = strdup(ctx->username.c_str());
            responses[i].resp_retcode = 0;
            break;

        case PAM_ERROR_MSG:
            // Forward PAM error to GUI
            if (ctx->manager) {
                QMetaObject::invokeMethod(ctx->manager, [manager = ctx->manager, promptText]() {
                    manager->handlePamError(promptText);
                }, Qt::QueuedConnection);
            }
            responses[i].resp = nullptr;
            responses[i].resp_retcode = 0;
            break;

        case PAM_TEXT_INFO:
            // Informational text: e.g. "Place finger on reader", "Swipe finger", "Face recognized"
            if (ctx->manager) {
                QMetaObject::invokeMethod(ctx->manager, [manager = ctx->manager, promptText]() {
                    manager->handlePamTextInfo(promptText);
                }, Qt::QueuedConnection);
            }
            responses[i].resp = nullptr;
            responses[i].resp_retcode = 0;
            break;

        default:
            responses[i].resp = nullptr;
            responses[i].resp_retcode = 0;
            break;
        }
    }

    *resp = responses;
    return PAM_SUCCESS;
}

void AuthManager::authenticate(const QString &password)
{
    if (m_isAuthenticating) {
        cancelAuthentication();
    }

    m_isAuthenticating = true;
    m_authError = false;
    m_authMessage = QStringLiteral("Authenticating...");
    m_abortRequested.store(false);
    emit authStateChanged();

    std::string userStr = m_username.toStdString();
    std::string passStr = password.toStdString();

    m_workerThread = std::make_unique<std::thread>([this, userStr, passStr]() {
        PamWorkerContext ctx{
            .username = userStr,
            .password = passStr,
            .abortFlag = &m_abortRequested,
            .manager = this
        };

        struct pam_conv conv{
            .conv = &AuthManager::pamConversationCallback,
            .appdata_ptr = &ctx
        };

        pam_handle_t *pamh = nullptr;

        // Try service name "axis-greeter", fall back to "login" or "common-auth"
        const char *serviceName = "axis-greeter";
        if (!QFile::exists(QStringLiteral("/etc/pam.d/axis-greeter"))) {
            serviceName = "login";
        }

        int ret = pam_start(serviceName, ctx.username.c_str(), &conv, &pamh);
        if (ret != PAM_SUCCESS) {
            // Fallback to "su" or "other"
            ret = pam_start("su", ctx.username.c_str(), &conv, &pamh);
        }

        if (ret == PAM_SUCCESS && pamh) {
            ret = pam_authenticate(pamh, 0);
            if (ret == PAM_SUCCESS) {
                ret = pam_acct_mgmt(pamh, 0);
            }
            pam_end(pamh, ret);
        }

        if (m_abortRequested.load()) {
            return;
        }

        QMetaObject::invokeMethod(this, [this, ret]() {
            handlePamResult(ret);
        }, Qt::QueuedConnection);
    });
}

void AuthManager::startIrisAuth()
{
    m_isFaceAuthActive = true;
    m_authMessage = QStringLiteral("Scanning iris via infrared camera...");
    emit faceAuthStateChanged();
    emit authStateChanged();
    authenticate(QString());
}

void AuthManager::startBiometricAuth()
{
    if (!m_hasFingerprint && !m_hasFaceAuth) {
        return;
    }
    // Initiate non-blocking PAM authentication without password to trigger biometric prompt
    authenticate(QString());
}

void AuthManager::cancelAuthentication()
{
    m_abortRequested.store(true);
    if (m_workerThread && m_workerThread->joinable()) {
        m_workerThread->join();
    }
    m_workerThread.reset();

    m_isAuthenticating = false;
    m_isFingerprintActive = false;
    m_isFaceAuthActive = false;
    emit authStateChanged();
    emit fingerprintStateChanged();
    emit faceAuthStateChanged();
}

void AuthManager::lockSession()
{
    setIsLocked(true);
}

void AuthManager::unlockSession()
{
    if (!m_isAuthenticated && !passwordlessUnlock()) {
        return;
    }
    setIsLocked(false);
}

void AuthManager::resetState()
{
    m_authError = false;
    m_authMessage.clear();
    m_isAuthenticating = false;
    m_isFingerprintActive = false;
    m_isFaceAuthActive = false;
    emit authStateChanged();
    emit fingerprintStateChanged();
    emit faceAuthStateChanged();
}

void AuthManager::handlePamTextInfo(const QString &msg)
{
    QString lower = msg.toLower();
    m_authMessage = msg;

    if (lower.contains(QStringLiteral("finger")) ||
        lower.contains(QStringLiteral("fprint")) ||
        lower.contains(QStringLiteral("reader")) ||
        lower.contains(QStringLiteral("swipe")) ||
        lower.contains(QStringLiteral("touch"))) {
        m_isFingerprintActive = true;
        emit fingerprintPulseRequested();
        emit fingerprintStateChanged();
    } else if (lower.contains(QStringLiteral("face")) ||
               lower.contains(QStringLiteral("camera")) ||
               lower.contains(QStringLiteral("howdy")) ||
               lower.contains(QStringLiteral("looking"))) {
        m_isFaceAuthActive = true;
        emit faceAuthStateChanged();
    }

    emit authStateChanged();
}

void AuthManager::handlePamError(const QString &msg)
{
    m_authMessage = msg;
    m_authError = true;
    emit authStateChanged();
}

void AuthManager::handlePamResult(int pamResult)
{
    m_isAuthenticating = false;
    m_isFingerprintActive = false;
    m_isFaceAuthActive = false;

    if (pamResult == PAM_SUCCESS) {
        m_isAuthenticated = true;
        m_isLocked = false;
        m_authError = false;
        m_authMessage = QStringLiteral("Access Granted");
        emit authenticated();
        emit lockStateChanged();
    } else {
        m_isAuthenticated = false;
        m_authError = true;
        if (m_authMessage.isEmpty() || !m_authMessage.contains(QStringLiteral("fail"), Qt::CaseInsensitive)) {
            m_authMessage = QStringLiteral("Authentication Failed");
        }
        emit authFailed(m_authMessage);
    }

    emit authStateChanged();
    emit fingerprintStateChanged();
    emit faceAuthStateChanged();
}
