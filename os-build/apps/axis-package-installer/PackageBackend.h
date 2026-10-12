#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QProcess>
#include <memory>

class PackageBackend : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isBusy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectionChanged)
    Q_PROPERTY(QString currentLog READ currentLog NOTIFY logChanged)

public:
    explicit PackageBackend(QObject *parent = nullptr);
    ~PackageBackend() override;

    bool isBusy() const { return m_isBusy; }
    QString statusMessage() const { return m_statusMessage; }
    int selectedCount() const { return m_selectedPackages.size(); }
    QString currentLog() const { return m_log; }

    Q_INVOKABLE QVariantList getCategories();
    Q_INVOKABLE QVariantList getPackagesForCategory(const QString &category);
    Q_INVOKABLE bool isPackageInstalled(const QString &pkgName);
    Q_INVOKABLE bool isPackageSelected(const QString &pkgName) const;
    Q_INVOKABLE void togglePackageSelection(const QString &pkgName);
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void installSelected();
    Q_INVOKABLE void cancelInstallation();

Q_SIGNALS:
    void busyChanged();
    void statusChanged();
    void selectionChanged();
    void logChanged();
    void installFinished(bool success);

private Q_SLOTS:
    void onProcessOutput();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void initPackageData();

    bool m_isBusy{false};
    QString m_statusMessage;
    QString m_log;
    QStringList m_selectedPackages;
    std::unique_ptr<QProcess> m_process;

    struct PackageItem {
        QString id;
        QString name;
        QString description;
        QString icon;
        QString category;
        bool installed{false};
    };

    QList<PackageItem> m_packages;
    QStringList m_categories;
};
