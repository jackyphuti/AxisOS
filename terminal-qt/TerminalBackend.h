#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QSocketNotifier>
#include <QColor>
#include <QVector>
#include <memory>
#include <pty.h>
#include <vterm.h>

struct TerminalCell {
    uint32_t character{0};
    QColor fgColor;
    QColor bgColor;
    bool bold{false};
    bool italic{false};
    bool underline{false};
    bool inverse{false};
    int width{1};
};

class TerminalBackend : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int rows READ rows NOTIFY sizeChanged)
    Q_PROPERTY(int cols READ cols NOTIFY sizeChanged)
    Q_PROPERTY(int cursorRow READ cursorRow NOTIFY cursorChanged)
    Q_PROPERTY(int cursorCol READ cursorCol NOTIFY cursorChanged)
    Q_PROPERTY(bool cursorVisible READ cursorVisible NOTIFY cursorChanged)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)

public:
    explicit TerminalBackend(int rows = 24, int cols = 80, QObject *parent = nullptr);
    ~TerminalBackend() override;

    bool startShell(const QString &shellProgram = QStringLiteral("/bin/bash"));
    void closeShell();

    int rows() const { return m_rows; }
    int cols() const { return m_cols; }
    int cursorRow() const { return m_cursorRow; }
    int cursorCol() const { return m_cursorCol; }
    bool cursorVisible() const { return m_cursorVisible; }
    QString title() const { return m_title; }

    // Screen buffer access
    TerminalCell getCell(int row, int col) const;
    void resize(int rows, int cols);

public Q_SLOTS:
    void sendInput(const QByteArray &data);
    void sendInput(const QString &text);

Q_SIGNALS:
    void screenUpdated();
    void cursorChanged();
    void sizeChanged();
    void titleChanged();
    void bellTriggered();
    void shellExited(int exitCode);

private Q_SLOTS:
    void onPtyDataAvailable();

private:
    void setupVTerm();
    void applyLiquidNodesPalette();
    static QColor vtermColorToQColor(const VTermColor &col);

    // libvterm callbacks
    static int onDamage(VTermRect rect, void *user);
    static int onMoverect(VTermRect dest, VTermRect src, void *user);
    static int onMovecursor(VTermPos pos, VTermPos oldpos, int visible, void *user);
    static int onSetTermProp(VTermProp prop, VTermValue *val, void *user);
    static int onBell(void *user);
    static int onResize(int rows, int cols, void *user);

    int m_rows{24};
    int m_cols{80};
    int m_cursorRow{0};
    int m_cursorCol{0};
    bool m_cursorVisible{true};
    QString m_title{QStringLiteral("Axis Terminal")};

    // PTY Process State
    int m_masterFd{-1};
    pid_t m_childPid{-1};
    QSocketNotifier *m_readNotifier{nullptr};

    // libvterm State
    VTerm *m_vterm{nullptr};
    VTermScreen *m_vtermScreen{nullptr};
};
