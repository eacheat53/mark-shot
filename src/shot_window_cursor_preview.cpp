#include "shot_window_module.h"

using namespace markshot::shot;

void ShotWindow::startWheelPreview(QPointF widgetPoint, WheelPreview preview)
{
    m_wheelPreview = preview;
    m_wheelPreviewPosition = widgetPoint;
    m_wheelPreviewTimer.restart();
    if (!m_wheelPreviewHideTimer) {
        m_wheelPreviewHideTimer = new QTimer(this);
        m_wheelPreviewHideTimer->setSingleShot(true);
        m_wheelPreviewHideTimer->setTimerType(Qt::PreciseTimer);
        connect(m_wheelPreviewHideTimer, &QTimer::timeout, this, &ShotWindow::clearWheelPreview);
    }
    // 1. 【标注】【指针预览】由独立定时器结束预览，恢复光标不依赖下一次移动或重绘
    m_wheelPreviewHideTimer->start(900);
    updatePointerCursor(widgetPoint);
    update();
}

bool ShotWindow::wheelPreviewVisible() const
{
    return m_wheelPreview != WheelPreview::None && m_wheelPreviewTimer.isValid()
        && m_wheelPreviewTimer.elapsed() <= 900 && m_mode == Mode::Editing
        && !m_operationBusy && !m_dragging && !m_imagePanning && !m_toolbarDragging
        && m_frozenImageRect.contains(m_wheelPreviewPosition)
        && !mouseOverUiWidget(m_wheelPreviewPosition) && !propertyComboPopupVisible();
}

void ShotWindow::clearWheelPreview()
{
    if (m_wheelPreviewHideTimer) {
        m_wheelPreviewHideTimer->stop();
    }
    if (m_wheelPreview == WheelPreview::None) {
        return;
    }
    m_wheelPreview = WheelPreview::None;
    m_wheelPreviewTimer.invalidate();
    updateCursor();
    update();
}

void ShotWindow::drawWheelPreview(QPainter &painter)
{
    if (!wheelPreviewVisible()) {
        return;
    }
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    if (m_wheelPreview == WheelPreview::ImageZoom) {
        const QString zoomText = QStringLiteral("%1%").arg(qRound(m_imageZoom * 100.0));
        painter.setFont(markshot::theme::uiFont(11, QFont::DemiBold));
        const QFontMetrics metrics(painter.font());
        const QRectF textBounds = metrics.boundingRect(zoomText);
        QRectF bubble(m_wheelPreviewPosition + QPointF(14, 14),
                       QSizeF(textBounds.width() + 20, textBounds.height() + 10));
        bubble.moveLeft(std::clamp(bubble.left(), 8.0, std::max(8.0, width() - bubble.width() - 8.0)));
        bubble.moveTop(std::clamp(bubble.top(), 8.0, std::max(8.0, height() - bubble.height() - 8.0)));
        painter.setPen(QPen(QColor(148, 163, 184, 80), 1));
        painter.setBrush(QColor(17, 24, 39, 235));
        painter.drawRoundedRect(bubble, 6, 6);
        painter.setPen(QColor(229, 231, 235));
        painter.drawText(bubble, Qt::AlignCenter, zoomText);
    } else {
        const qreal extent = std::clamp(currentToolPreviewSize(), 2.0, 96.0);
        const QRectF preview(m_wheelPreviewPosition - QPointF(extent / 2, extent / 2), QSizeF(extent, extent));
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(17, 24, 39, 235), 3));
        painter.drawEllipse(preview);
        painter.setPen(QPen(m_currentColor, 1));
        painter.drawEllipse(preview);
        // 1. 【标注】【指针预览】尺寸轮廓不遮挡画面，中心使用与系统一致的精确十字
        drawCaptureCrossCursor(painter, m_wheelPreviewPosition);
    }
    painter.restore();
}
