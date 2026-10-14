#include "TerminalScreenItem.h"

#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QClipboard>
#include <QGuiApplication>
#include <QDebug>
#include <algorithm>

TerminalScreenItem::TerminalScreenItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    setFlag(ItemAcceptsInputMethod, true);
    setFlag(ItemIsFocusScope, true);
    setAcceptedMouseButtons(Qt::AllButtons);

    m_font = QFont(m_fontFamily, m_fontSize);
    m_font.setStyleHint(QFont::Monospace);
    m_font.setFixedPitch(true);
    updateFontMetrics();

    m_blinkTimer.setInterval(520);
    connect(&m_blinkTimer, &QTimer::timeout, this, &TerminalScreenItem::toggleCursorBlink);
    m_blinkTimer.start();
}

TerminalScreenItem::~TerminalScreenItem() = default;

void TerminalScreenItem::setBackend(TerminalBackend *backend)
{
    if (m_backend != backend) {
        if (m_backend) {
            disconnect(m_backend, nullptr, this, nullptr);
        }
        m_backend = backend;
        if (m_backend) {
            connect(m_backend, &TerminalBackend::screenUpdated, this, &TerminalScreenItem::onScreenUpdated);
            connect(m_backend, &TerminalBackend::cursorChanged, this, &TerminalScreenItem::onScreenUpdated);
            connect(m_backend, &TerminalBackend::sizeChanged, this, &TerminalScreenItem::onScreenUpdated);

            // Resize PTY to match initial dimensions
            if (width() > 0 && height() > 0) {
                int cols = std::max(10, static_cast<int>(width() / m_charWidth));
                int rows = std::max(5, static_cast<int>(height() / m_charHeight));
                m_backend->resize(rows, cols);
            }
        }
        emit backendChanged();
        m_dirty = true;
        update();
    }
}

void TerminalScreenItem::setFontFamily(const QString &family)
{
    if (m_fontFamily != family) {
        m_fontFamily = family;
        m_font.setFamily(family);
        updateFontMetrics();
        emit fontChanged();
    }
}

void TerminalScreenItem::setFontSize(int size)
{
    if (m_fontSize != size && size >= 6 && size <= 48) {
        m_fontSize = size;
        m_font.setPointSize(size);
        updateFontMetrics();
        emit fontChanged();
    }
}

void TerminalScreenItem::setCursorBlink(bool blink)
{
    if (m_cursorBlink != blink) {
        m_cursorBlink = blink;
        if (m_cursorBlink) {
            m_blinkTimer.start();
        } else {
            m_blinkTimer.stop();
            m_cursorBlinkState = true;
        }
        emit cursorBlinkChanged();
        update();
    }
}

void TerminalScreenItem::updateFontMetrics()
{
    QFontMetricsF fm(m_font);
    m_charWidth = fm.horizontalAdvance(QLatin1Char('M'));
    m_charHeight = fm.lineSpacing();
    m_fontAscent = fm.ascent();

    if (m_charWidth < 1.0) m_charWidth = 9.0;
    if (m_charHeight < 1.0) m_charHeight = 18.0;

    emit metricsChanged();

    if (m_backend && width() > 0 && height() > 0) {
        int cols = std::max(10, static_cast<int>(width() / m_charWidth));
        int rows = std::max(5, static_cast<int>(height() / m_charHeight));
        m_backend->resize(rows, cols);
    }

    m_dirty = true;
    update();
}

void TerminalScreenItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);

    if (newGeometry.size() != oldGeometry.size() && m_backend) {
        int cols = std::max(10, static_cast<int>(newGeometry.width() / m_charWidth));
        int rows = std::max(5, static_cast<int>(newGeometry.height() / m_charHeight));
        m_backend->resize(rows, cols);
        m_dirty = true;
        update();
    }
}

void TerminalScreenItem::onScreenUpdated()
{
    m_dirty = true;
    update();
}

void TerminalScreenItem::toggleCursorBlink()
{
    if (m_cursorBlink) {
        m_cursorBlinkState = !m_cursorBlinkState;
        update();
    }
}

void TerminalScreenItem::mousePressEvent(QMouseEvent *event)
{
    forceActiveFocus();
    event->accept();
}

void TerminalScreenItem::keyPressEvent(QKeyEvent *event)
{
    if (!m_backend) {
        event->ignore();
        return;
    }

    const bool ctrl = event->modifiers() & Qt::ControlModifier;
    const bool shift = event->modifiers() & Qt::ShiftModifier;
    const bool alt = event->modifiers() & Qt::AltModifier;

    // Clipboard shortcuts: Ctrl+Shift+C / Ctrl+Shift+V
    if (ctrl && shift && event->key() == Qt::Key_C) {
        copyToClipboard();
        event->accept();
        return;
    }
    if (ctrl && shift && event->key() == Qt::Key_V) {
        pasteFromClipboard();
        event->accept();
        return;
    }

    // Font size scaling: Ctrl++ / Ctrl+-
    if (ctrl && (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal)) {
        setFontSize(m_fontSize + 1);
        event->accept();
        return;
    }
    if (ctrl && event->key() == Qt::Key_Minus) {
        setFontSize(m_fontSize - 1);
        event->accept();
        return;
    }

    QByteArray seq;

    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        seq = "\r";
        break;
    case Qt::Key_Backspace:
        seq = "\x7f";
        break;
    case Qt::Key_Tab:
        seq = "\t";
        break;
    case Qt::Key_Backtab:
        seq = "\x1b[Z";
        break;
    case Qt::Key_Escape:
        seq = "\x1b";
        break;
    case Qt::Key_Up:
        seq = alt ? "\x1b\x1b[A" : (ctrl ? "\x1b[1;5A" : "\x1b[A");
        break;
    case Qt::Key_Down:
        seq = alt ? "\x1b\x1b[B" : (ctrl ? "\x1b[1;5B" : "\x1b[B");
        break;
    case Qt::Key_Right:
        seq = alt ? "\x1b\x1b[C" : (ctrl ? "\x1b[1;5C" : "\x1b[C");
        break;
    case Qt::Key_Left:
        seq = alt ? "\x1b\x1b[D" : (ctrl ? "\x1b[1;5D" : "\x1b[D");
        break;
    case Qt::Key_Home:
        seq = "\x1b[H";
        break;
    case Qt::Key_End:
        seq = "\x1b[F";
        break;
    case Qt::Key_PageUp:
        seq = "\x1b[5~";
        break;
    case Qt::Key_PageDown:
        seq = "\x1b[6~";
        break;
    case Qt::Key_Delete:
        seq = "\x1b[3~";
        break;
    case Qt::Key_Insert:
        seq = "\x1b[2~";
        break;
    case Qt::Key_F1:  seq = "\x1bOP"; break;
    case Qt::Key_F2:  seq = "\x1bOQ"; break;
    case Qt::Key_F3:  seq = "\x1bOR"; break;
    case Qt::Key_F4:  seq = "\x1bOS"; break;
    case Qt::Key_F5:  seq = "\x1b[15~"; break;
    case Qt::Key_F6:  seq = "\x1b[17~"; break;
    case Qt::Key_F7:  seq = "\x1b[18~"; break;
    case Qt::Key_F8:  seq = "\x1b[19~"; break;
    case Qt::Key_F9:  seq = "\x1b[20~"; break;
    case Qt::Key_F10: seq = "\x1b[21~"; break;
    case Qt::Key_F11: seq = "\x1b[23~"; break;
    case Qt::Key_F12: seq = "\x1b[24~"; break;
    default:
        break;
    }

    if (!seq.isEmpty()) {
        m_backend->sendInput(seq);
        event->accept();
        return;
    }

    // Ctrl + Key combinations (e.g. Ctrl+C -> 0x03, Ctrl+D -> 0x04, Ctrl+Z -> 0x1A)
    if (ctrl && !shift && !alt) {
        int key = event->key();
        if (key >= Qt::Key_A && key <= Qt::Key_Z) {
            char code = static_cast<char>(key - Qt::Key_A + 1);
            m_backend->sendInput(QByteArray(1, code));
            event->accept();
            return;
        } else if (key == Qt::Key_Space || key == Qt::Key_At) {
            m_backend->sendInput(QByteArray(1, '\0'));
            event->accept();
            return;
        }
    }

    // Regular Unicode text input
    QString text = event->text();
    if (!text.isEmpty()) {
        m_backend->sendInput(text.toUtf8());
        event->accept();
        return;
    }

    event->ignore();
}

void TerminalScreenItem::copyToClipboard()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (!clipboard || !m_backend) return;

    QStringList lines;
    const int totalRows = m_backend->rows();
    const int totalCols = m_backend->cols();

    for (int r = 0; r < totalRows; ++r) {
        QString rowStr;
        rowStr.reserve(totalCols);
        for (int c = 0; c < totalCols; ++c) {
            TerminalCell cell = m_backend->getCell(r, c);
            if (cell.character > 0) {
                const char32_t ucs = static_cast<char32_t>(cell.character);
                rowStr.append(QString::fromUcs4(&ucs, 1));
            } else {
                rowStr.append(QLatin1Char(' '));
            }
            if (cell.width > 1) {
                c += (cell.width - 1);
            }
        }
        while (rowStr.endsWith(QLatin1Char(' '))) {
            rowStr.chop(1);
        }
        lines.append(rowStr);
    }

    while (!lines.isEmpty() && lines.last().isEmpty()) {
        lines.removeLast();
    }
    clipboard->setText(lines.join(QLatin1Char('\n')));
}

void TerminalScreenItem::pasteFromClipboard()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard && m_backend) {
        QString text = clipboard->text();
        if (!text.isEmpty()) {
            m_backend->sendInput(text.toUtf8());
        }
    }
}

void TerminalScreenItem::renderScreenImage()
{
    const int imgW = std::max(1, static_cast<int>(width()));
    const int imgH = std::max(1, static_cast<int>(height()));

    if (m_screenImage.size() != QSize(imgW, imgH)) {
        m_screenImage = QImage(imgW, imgH, QImage::Format_ARGB32_Premultiplied);
    }

    // Transparent Obsidian base for frosted glassmorphism behind terminal
    m_screenImage.fill(QColor(0x06, 0x07, 0x0A, 220));

    if (!m_backend) return;

    QPainter painter(&m_screenImage);
    painter.setFont(m_font);

    const int totalRows = m_backend->rows();
    const int totalCols = m_backend->cols();

    for (int r = 0; r < totalRows; ++r) {
        qreal y = r * m_charHeight;
        if (y >= imgH) break;

        for (int c = 0; c < totalCols; ++c) {
            qreal x = c * m_charWidth;
            if (x >= imgW) break;

            TerminalCell cell = m_backend->getCell(r, c);

            // Draw background fill if not default obsidian
            if (cell.bgColor.isValid() && cell.bgColor.rgb() != QColor(0x06, 0x07, 0x0A).rgb()) {
                painter.fillRect(QRectF(x, y, m_charWidth * cell.width, m_charHeight), cell.bgColor);
            }

            // Draw text character
            if (cell.character > 0 && cell.character != ' ') {
                const char32_t ucsChar = static_cast<char32_t>(cell.character);
                QString chStr = QString::fromUcs4(&ucsChar, 1);
                QFont f = m_font;
                if (cell.bold) f.setBold(true);
                if (cell.italic) f.setItalic(true);
                painter.setFont(f);
                painter.setPen(cell.fgColor.isValid() ? cell.fgColor : QColor("#F8FAFC"));
                painter.drawText(QPointF(x, y + m_fontAscent), chStr);

                if (cell.underline) {
                    painter.drawLine(QPointF(x, y + m_charHeight - 1), QPointF(x + m_charWidth, y + m_charHeight - 1));
                }
            }

            if (cell.width > 1) {
                c += (cell.width - 1);
            }
        }
    }

    // Draw glowing cyber-teal terminal cursor
    if (hasActiveFocus() && m_cursorBlinkState && m_backend->cursorVisible()) {
        int cr = m_backend->cursorRow();
        int cc = m_backend->cursorCol();
        if (cr >= 0 && cr < totalRows && cc >= 0 && cc < totalCols) {
            qreal cx = cc * m_charWidth;
            qreal cy = cr * m_charHeight;

            // Neon Cyber-Teal cursor box
            painter.fillRect(QRectF(cx, cy, m_charWidth, m_charHeight), QColor(0x00, 0xF2, 0xFE, 180));
            // Cursor inner border
            painter.setPen(QColor(0x00, 0xF2, 0xFE));
            painter.drawRect(QRectF(cx, cy, m_charWidth, m_charHeight));

            // Invert character under cursor
            TerminalCell underCursor = m_backend->getCell(cr, cc);
            if (underCursor.character > 0 && underCursor.character != ' ') {
                const char32_t cursorUcs = static_cast<char32_t>(underCursor.character);
                QString chStr = QString::fromUcs4(&cursorUcs, 1);
                painter.setPen(QColor("#06070A"));
                painter.drawText(QPointF(cx, cy + m_fontAscent), chStr);
            }
        }
    }

    painter.end();
}

QSGNode *TerminalScreenItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    if (width() <= 0 || height() <= 0) {
        delete oldNode;
        return nullptr;
    }

    if (m_dirty) {
        renderScreenImage();
        m_dirty = false;
    }

    auto *node = static_cast<QSGSimpleTextureNode *>(oldNode);
    if (!node) {
        node = new QSGSimpleTextureNode();
    }

    QSGTexture *texture = window()->createTextureFromImage(m_screenImage);
    node->setTexture(texture);
    node->setOwnsTexture(true);
    node->setRect(boundingRect());
    node->setFiltering(QSGTexture::Nearest);

    return node;
}
