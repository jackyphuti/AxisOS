#include "TerminalBackend.h"

#include <QDebug>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <csignal>
#include <cerrno>

TerminalBackend::TerminalBackend(int rows, int cols, QObject *parent)
    : QObject(parent)
    , m_rows(rows)
    , m_cols(cols)
{
    setupVTerm();
}

TerminalBackend::~TerminalBackend()
{
    closeShell();
    if (m_vterm) {
        vterm_free(m_vterm);
        m_vterm = nullptr;
    }
}

void TerminalBackend::setupVTerm()
{
    m_vterm = vterm_new(m_rows, m_cols);
    vterm_set_utf8(m_vterm, 1);

    m_vtermScreen = vterm_obtain_screen(m_vterm);
    vterm_screen_reset(m_vtermScreen, 1);

    static VTermScreenCallbacks callbacks = {
        .damage = &TerminalBackend::onDamage,
        .moverect = &TerminalBackend::onMoverect,
        .movecursor = &TerminalBackend::onMovecursor,
        .settermprop = &TerminalBackend::onSetTermProp,
        .bell = &TerminalBackend::onBell,
        .resize = &TerminalBackend::onResize,
        .sb_pushline = nullptr,
        .sb_popline = nullptr
    };

    vterm_screen_set_callbacks(m_vtermScreen, &callbacks, this);
    applyLiquidNodesPalette();
}

void TerminalBackend::applyLiquidNodesPalette()
{
    VTermState *state = vterm_obtain_state(m_vterm);
    if (!state) return;

    // Unified AxisOS One UI Dark 16-Color Palette
    const uint32_t palette[16] = {
        0x141826, // 0: Base Navy
        0xF43F5E, // 1: Danger Rose
        0x2BB673, // 2: Success Emerald
        0xF59E0B, // 3: Warning Amber
        0x7D95FF, // 4: Accent Blue
        0xA78BFA, // 5: Soft Violet
        0x38BDF8, // 6: Sky Cyan
        0xF2F3F7, // 7: Text Primary
        // Bright variations
        0x2E3349, // 8: Divider Slate
        0xFB7185, // 9: Bright Rose
        0x4ADE80, // 10: Bright Green
        0xFBBF24, // 11: Bright Amber
        0x93C5FD, // 12: Bright Blue
        0xC4B5FD, // 13: Bright Violet
        0x7DD3FC, // 14: Bright Cyan
        0xFFFFFF  // 15: Pure White
    };

    for (int i = 0; i < 16; ++i) {
        VTermColor col;
        uint8_t r = (palette[i] >> 16) & 0xFF;
        uint8_t g = (palette[i] >> 8) & 0xFF;
        uint8_t b = palette[i] & 0xFF;
        vterm_color_rgb(&col, r, g, b);
        vterm_state_set_palette_color(state, i, &col);
    }

    // Default Foreground & Background (Unified One UI Dark)
    VTermColor defFg, defBg;
    vterm_color_rgb(&defFg, 0xF2, 0xF3, 0xF7); // Text Primary #F2F3F7
    vterm_color_rgb(&defBg, 0x14, 0x18, 0x26); // Base #141826
    vterm_state_set_default_colors(state, &defFg, &defBg);
}

bool TerminalBackend::startShell(const QString &shellProgram, const QString &command)
{
    closeShell();

    struct winsize ws{};
    ws.ws_row = static_cast<unsigned short>(m_rows);
    ws.ws_col = static_cast<unsigned short>(m_cols);
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;

    m_childPid = forkpty(&m_masterFd, nullptr, nullptr, &ws);

    if (m_childPid < 0) {
        qCritical() << "[AxisTerminal] forkpty() failed:" << strerror(errno);
        return false;
    }

    if (m_childPid == 0) {
        // Child Process: Configure terminal environment and launch shell
        setenv("TERM", "xterm-256color", 1);
        setenv("COLORTERM", "truecolor", 1);
        setenv("LANG", "en_US.UTF-8", 1);
        setenv("LC_ALL", "en_US.UTF-8", 1);

        QByteArray progBytes = shellProgram.toUtf8();
        const char *prog = progBytes.constData();

        if (!command.trimmed().isEmpty()) {
            QByteArray cmdBytes = command.toUtf8();
            execl(prog, prog, "-c", cmdBytes.constData(), nullptr);
            execl("/bin/sh", "/bin/sh", "-c", cmdBytes.constData(), nullptr);
            _exit(127);
        }

        execl(prog, prog, "-i", nullptr);

        // Fallback to /bin/sh if primary shell fails
        execl("/bin/sh", "/bin/sh", "-i", nullptr);
        _exit(127);
    }

    // Parent Process: Configure non-blocking PTY master descriptor
    int flags = fcntl(m_masterFd, F_GETFL, 0);
    fcntl(m_masterFd, F_SETFL, flags | O_NONBLOCK);

    m_readNotifier = new QSocketNotifier(m_masterFd, QSocketNotifier::Read, this);
    connect(m_readNotifier, &QSocketNotifier::activated, this, &TerminalBackend::onPtyDataAvailable);

    qInfo() << "[AxisTerminal] PTY shell spawned successfully with PID:" << m_childPid
            << "masterFd:" << m_masterFd;

    return true;
}

void TerminalBackend::closeShell()
{
    if (m_readNotifier) {
        m_readNotifier->setEnabled(false);
        delete m_readNotifier;
        m_readNotifier = nullptr;
    }

    if (m_masterFd >= 0) {
        close(m_masterFd);
        m_masterFd = -1;
    }

    if (m_childPid > 0) {
        kill(m_childPid, SIGHUP);
        int status = 0;
        pid_t res = waitpid(m_childPid, &status, WNOHANG);
        if (res == 0) {
            usleep(25000);
            if (waitpid(m_childPid, &status, WNOHANG) == 0) {
                kill(m_childPid, SIGTERM);
                waitpid(m_childPid, &status, 0);
            }
        }
        m_childPid = -1;
    }
}

void TerminalBackend::onPtyDataAvailable()
{
    if (m_masterFd < 0 || !m_vterm) return;

    char buffer[8192];
    ssize_t bytesRead = 0;

    while ((bytesRead = ::read(m_masterFd, buffer, sizeof(buffer))) > 0) {
        vterm_input_write(m_vterm, buffer, static_cast<size_t>(bytesRead));
        analyzeOutputForSelfHealing(QString::fromUtf8(buffer, bytesRead));
    }

    if (bytesRead < 0 && (errno != EAGAIN && errno != EWOULDBLOCK)) {
        if (errno != EIO) {
            qWarning() << "[AxisTerminal] PTY read error:" << strerror(errno);
        }
        closeShell();
        emit shellExited(0);
        return;
    } else if (bytesRead == 0) {
        // Child closed PTY (e.g. exit command typed)
        closeShell();
        emit shellExited(0);
        return;
    }

    emit screenUpdated();
}

void TerminalBackend::sendInput(const QByteArray &data)
{
    if (m_masterFd >= 0 && !data.isEmpty()) {
        ssize_t written = ::write(m_masterFd, data.constData(), data.size());
        Q_UNUSED(written);
    }
}

void TerminalBackend::sendInput(const QString &text)
{
    sendInput(text.toUtf8());
}

void TerminalBackend::resize(int rows, int cols)
{
    if (rows <= 0 || cols <= 0) return;
    if (rows == m_rows && cols == m_cols) return;

    m_rows = rows;
    m_cols = cols;

    if (m_vterm) {
        vterm_set_size(m_vterm, m_rows, m_cols);
    }

    if (m_masterFd >= 0) {
        struct winsize ws{};
        ws.ws_row = static_cast<unsigned short>(m_rows);
        ws.ws_col = static_cast<unsigned short>(m_cols);
        ::ioctl(m_masterFd, TIOCSWINSZ, &ws);
    }

    emit sizeChanged();
    emit screenUpdated();
}

TerminalCell TerminalBackend::getCell(int row, int col) const
{
    TerminalCell result;
    if (!m_vtermScreen || row < 0 || row >= m_rows || col < 0 || col >= m_cols) {
        return result;
    }

    VTermPos pos{row, col};
    VTermScreenCell cell;
    if (vterm_screen_get_cell(m_vtermScreen, pos, &cell) == 0) {
        return result;
    }

    result.character = cell.chars[0];
    result.width = cell.width;
    result.bold = cell.attrs.bold;
    result.italic = cell.attrs.italic;
    result.underline = (cell.attrs.underline != 0);
    result.inverse = cell.attrs.reverse;

    // Convert colors to RGB
    vterm_screen_convert_color_to_rgb(m_vtermScreen, &cell.fg);
    vterm_screen_convert_color_to_rgb(m_vtermScreen, &cell.bg);

    result.fgColor = vtermColorToQColor(cell.fg);
    result.bgColor = vtermColorToQColor(cell.bg);

    if (result.inverse) {
        std::swap(result.fgColor, result.bgColor);
    }

    return result;
}

QColor TerminalBackend::vtermColorToQColor(const VTermColor &col)
{
    return QColor::fromRgb(col.rgb.red, col.rgb.green, col.rgb.blue);
}

// -----------------------------------------------------------------------------
// libvterm C Callbacks
// -----------------------------------------------------------------------------
int TerminalBackend::onDamage(VTermRect rect, void *user)
{
    Q_UNUSED(rect);
    auto *self = static_cast<TerminalBackend *>(user);
    if (self) {
        emit self->screenUpdated();
    }
    return 1;
}

int TerminalBackend::onMoverect(VTermRect dest, VTermRect src, void *user)
{
    Q_UNUSED(dest);
    Q_UNUSED(src);
    auto *self = static_cast<TerminalBackend *>(user);
    if (self) {
        emit self->screenUpdated();
    }
    return 1;
}

int TerminalBackend::onMovecursor(VTermPos pos, VTermPos oldpos, int visible, void *user)
{
    Q_UNUSED(oldpos);
    auto *self = static_cast<TerminalBackend *>(user);
    if (self) {
        self->m_cursorRow = pos.row;
        self->m_cursorCol = pos.col;
        self->m_cursorVisible = (visible != 0);
        emit self->cursorChanged();
        emit self->screenUpdated();
    }
    return 1;
}

int TerminalBackend::onSetTermProp(VTermProp prop, VTermValue *val, void *user)
{
    auto *self = static_cast<TerminalBackend *>(user);
    if (!self) return 0;

    if (prop == VTERM_PROP_TITLE && val && val->string) {
        self->m_title = QString::fromUtf8(val->string);
        emit self->titleChanged();
    }
    return 1;
}

int TerminalBackend::onBell(void *user)
{
    auto *self = static_cast<TerminalBackend *>(user);
    if (self) {
        emit self->bellTriggered();
    }
    return 1;
}

int TerminalBackend::onResize(int rows, int cols, void *user)
{
    auto *self = static_cast<TerminalBackend *>(user);
    if (self) {
        self->m_rows = rows;
        self->m_cols = cols;
        emit self->sizeChanged();
        emit self->screenUpdated();
    }
    return 1;
}

void TerminalBackend::analyzeOutputForSelfHealing(const QString &chunk)
{
    QString lower = chunk.toLower();

    if (lower.contains(QStringLiteral("could not get lock /var/lib/dpkg")) ||
        lower.contains(QStringLiteral("unable to lock directory /var/lib/apt"))) {
        m_hasDiagnostic = true;
        m_diagnosticTitle = QStringLiteral("APT / Dpkg Lock Contention Detected");
        m_diagnosticFixCommand = QStringLiteral("sudo rm -f /var/lib/dpkg/lock-frontend /var/lib/apt/lists/lock && sudo dpkg --configure -a");
        emit diagnosticChanged();
    } else if (lower.contains(QStringLiteral("command not found"))) {
        // Extract missing binary name if possible
        QString missingCmd = QStringLiteral("package");
        int idx = chunk.indexOf(QStringLiteral(": command not found"));
        if (idx > 0) {
            QString prefix = chunk.left(idx).trimmed();
            int lastColon = prefix.lastIndexOf(QLatin1Char(':'));
            missingCmd = (lastColon >= 0) ? prefix.mid(lastColon + 1).trimmed() : prefix.section(QLatin1Char(' '), -1);
        }
        m_hasDiagnostic = true;
        m_diagnosticTitle = QStringLiteral("Missing Command: '") + missingCmd + QStringLiteral("'");
        m_diagnosticFixCommand = QStringLiteral("sudo axis install ") + missingCmd;
        emit diagnosticChanged();
    } else if (lower.contains(QStringLiteral("eaddrinuse")) ||
               lower.contains(QStringLiteral("address already in use"))) {
        m_hasDiagnostic = true;
        m_diagnosticTitle = QStringLiteral("Network Port Conflict (EADDRINUSE)");
        m_diagnosticFixCommand = QStringLiteral("sudo fuser -k 3000/tcp 2>/dev/null || lsof -i :3000");
        emit diagnosticChanged();
    } else if (lower.contains(QStringLiteral("error while loading shared libraries"))) {
        m_hasDiagnostic = true;
        m_diagnosticTitle = QStringLiteral("Missing Shared Library (.so) Cache");
        m_diagnosticFixCommand = QStringLiteral("sudo ldconfig && sudo apt-get -o DPkg::Lock::Timeout=120 install -f -y");
        emit diagnosticChanged();
    } else if (lower.contains(QStringLiteral("no space left on device"))) {
        m_hasDiagnostic = true;
        m_diagnosticTitle = QStringLiteral("Storage Full (ENOSPC)");
        m_diagnosticFixCommand = QStringLiteral("sudo apt-get clean && sudo journalctl --vacuum-size=100M");
        emit diagnosticChanged();
    } else if (lower.contains(QStringLiteral("permission denied"))) {
        m_hasDiagnostic = true;
        m_diagnosticTitle = QStringLiteral("Permission Denied — Elevate Command");
        m_diagnosticFixCommand = QStringLiteral("sudo !!");
        emit diagnosticChanged();
    }
}

void TerminalBackend::applyDiagnosticFix()
{
    if (!m_hasDiagnostic || m_diagnosticFixCommand.isEmpty()) return;
    QString cmd = m_diagnosticFixCommand + QStringLiteral("\r");
    m_hasDiagnostic = false;
    emit diagnosticChanged();
    sendInput(cmd.toUtf8());
}

void TerminalBackend::dismissDiagnostic()
{
    if (m_hasDiagnostic) {
        m_hasDiagnostic = false;
        emit diagnosticChanged();
    }
}
