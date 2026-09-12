#include "capture_cross_cursor.h"

#include <QPainter>
#include <QPixmap>

namespace markshot::shot {

void drawCaptureCrossCursor(QPainter &painter, QPointF position)
{
    // 1. 【标注】【十字光标】热点对齐像素中心，四臂留出中心取样区域
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.translate(QPointF(position.toPoint()) + QPointF(0.5, 0.5));
    const QLineF arms[] = {
        QLineF(-9, 0, -3, 0), QLineF(3, 0, 9, 0),
        QLineF(0, -9, 0, -3), QLineF(0, 3, 0, 9),
    };
    // 2. 【标注】【十字光标】浅色细线与深色轮廓在明暗背景上保持一致，不依赖标注颜色
    painter.setPen(QPen(QColor(17, 24, 39, 235), 3, Qt::SolidLine, Qt::RoundCap));
    painter.drawLines(arms, 4);
    painter.setPen(QPen(QColor(243, 244, 246), 1, Qt::SolidLine, Qt::RoundCap));
    painter.drawLines(arms, 4);
    painter.restore();
}

QRect captureCrossCursorRect(QPointF position)
{
    return QRect(position.toPoint() - QPoint(12, 12), QSize(25, 25));
}

QCursor captureCrossCursor()
{
    // 1. 【标注】【十字光标】缓存图案并保留 256 字节行步长，避免硬件光标出现错位
    static const QCursor cursor = [] {
        constexpr int canvasSize = 64;
        const QPoint hotspot(canvasSize / 2, canvasSize / 2);
        QPixmap pixmap(canvasSize, canvasSize);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        drawCaptureCrossCursor(painter, hotspot);
        painter.end();
        return QCursor(pixmap, hotspot.x(), hotspot.y());
    }();
    return cursor;
}

}
