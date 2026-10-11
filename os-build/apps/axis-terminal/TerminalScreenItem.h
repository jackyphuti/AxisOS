#pragma once

#include <QQuickItem>
#include <QFont>
#include <QFontMetricsF>
#include <QImage>
#include <QTimer>
#include <QSGSimpleTextureNode>
#include <QQuickWindow>

#include "TerminalBackend.h"

class TerminalScreenItem : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(TerminalBackend* backend READ backend WRITE setBackend NOTIFY backendChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY fontChanged)
    Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY fontChanged)
    Q_PROPERTY(qreal charWidth READ charWidth NOTIFY metricsChanged)
    Q_PROPERTY(qreal charHeight READ charHeight NOTIFY metricsChanged)
    Q_PROPERTY(bool cursorBlink READ cursorBlink WRITE setCursorBlink NOTIFY cursorBlinkChanged)

public:
    explicit TerminalScreenItem(QQuickItem *parent = nullptr);
    ~TerminalScreenItem() override;

    TerminalBackend *backend() const { return m_backend; }
    void setBackend(TerminalBackend *backend);

    QString fontFamily() const { return m_fontFamily; }
    void setFontFamily(const QString &family);

    int fontSize() const { return m_fontSize; }
    void setFontSize(int size);

    qreal charWidth() const { return m_charWidth; }
    qreal charHeight() const { return m_charHeight; }

    bool cursorBlink() const { return m_cursorBlink; }
    void setCursorBlink(bool blink);

public Q_SLOTS:
    void copyToClipboard();
    void pasteFromClipboard();

Q_SIGNALS:
    void backendChanged();
    void fontChanged();
    void metricsChanged();
    void cursorBlinkChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private Q_SLOTS:
    void onScreenUpdated();
    void toggleCursorBlink();

private:
    void updateFontMetrics();
    void renderScreenImage();

    TerminalBackend *m_backend{nullptr};
    QString m_fontFamily{QStringLiteral("monospace")};
    int m_fontSize{12};
    QFont m_font;
    qreal m_charWidth{9.0};
    qreal m_charHeight{18.0};
    qreal m_fontAscent{14.0};

    bool m_cursorBlink{true};
    bool m_cursorBlinkState{true};
    QTimer m_blinkTimer;

    QImage m_screenImage;
    bool m_dirty{true};
};
