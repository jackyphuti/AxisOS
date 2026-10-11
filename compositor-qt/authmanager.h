#pragma once

#include <QObject>
#include <QString>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <security/pam_appl.h>

class AuthManager : public QObject
{
    Q_OBJECT

    // Dynamic Hardware Capability Properties
    Q_PROPERTY(bool hasFingerprint READ hasFingerprint NOTIFY hardwareChanged)
    Q_PROPERTY(bool hasFaceAuth READ hasFaceAuth NOTIFY hardwareChanged)

    // Live Authentication State
    Q_PROPERTY(bool isAuthenticating READ isAuthenticating NOTIFY authStateChanged)
    Q_PROPERTY(bool isFingerprintActive READ isFingerprintActive NOTIFY fingerprintStateChanged)
    Q_PROPERTY(bool isFaceAuthActive READ isFaceAuthActive NOTIFY faceAuthStateChanged)
    Q_PROPERTY(QString authMessage READ authMessage NOTIFY authStateChanged)
    Q_PROPERTY(bool authError READ authError NOTIFY authStateChanged)

    // User & Session Info
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
    Q_PROPERTY(QString displayName READ displayName NOTIFY usernameChanged)
    Q_PROPERTY(bool isAuthenticated READ isAuthenticated NOTIFY authStateChanged)
    Q_PROPERTY(bool isLocked READ isLocked WRITE setIsLocked NOTIFY lockStateChanged)

public:
    explicit AuthManager(QObject *parent = nullptr);
    ~AuthManager() override;

    // Property getters
    bool hasFingerprint() const { return m_hasFingerprint; }
    bool hasFaceAuth() const { return m_hasFaceAuth; }
    bool isAuthenticating() const { return m_isAuthenticating; }
    bool isFingerprintActive() const { return m_isFingerprintActive; }
    bool isFaceAuthActive() const { return m_isFaceAuthActive; }
    QString authMessage() const { return m_authMessage; }
    bool authError() const { return m_authError; }
    QString username() const { return m_username; }
    QString displayName() const { return m_displayName; }
    bool isAuthenticated() const { return m_isAuthenticated; }
    bool isLocked() const { return m_isLocked; }

    void setUsername(const QString &user);
    void setIsLocked(bool locked);

public Q_SLOTS:
    // QML-invokable methods
    void checkHardware();
    void authenticate(const QString &password);
    void startBiometricAuth();
    void cancelAuthentication();
    void lockSession();
    void unlockSession();
    void resetState();

Q_SIGNALS:
    void hardwareChanged();
    void authStateChanged();
    void usernameChanged();
    void lockStateChanged();
    void fingerprintStateChanged();
    void faceAuthStateChanged();
    void fingerprintPulseRequested();
    void authenticated();
    void authFailed(const QString &errorMessage);

private:
    // Internal hardware probes
    bool detectFingerprintHardware();
    bool detectFaceAuthHardware();

    // Internal PAM lifecycle handlers called on GUI thread
    void handlePamTextInfo(const QString &msg);
    void handlePamError(const QString &msg);
    void handlePamResult(int pamResult);

    // Dynamic Hardware State
    bool m_hasFingerprint{false};
    bool m_hasFaceAuth{false};

    // Live Authentication & Greeter State
    bool m_isAuthenticating{false};
    bool m_isFingerprintActive{false};
    bool m_isFaceAuthActive{false};
    QString m_authMessage;
    bool m_authError{false};

    // User details
    QString m_username{QStringLiteral("axis")};
    QString m_displayName{QStringLiteral("AxisOS User")};
    bool m_isAuthenticated{false};
    bool m_isLocked{true};

    // PAM worker thread synchronization
    std::unique_ptr<std::thread> m_workerThread;
    std::atomic<bool> m_abortRequested{false};

    // PAM Conversation Context
    struct PamWorkerContext {
        std::string username;
        std::string password;
        std::atomic<bool> *abortFlag{nullptr};
        AuthManager *manager{nullptr};
    };

    static int pamConversationCallback(int num_msg,
                                       const struct pam_message **msg,
                                       struct pam_response **resp,
                                       void *appdata_ptr);
};
