#include "capture_cross_cursor.h"
#include "selection_loupe.h"

#include <QImage>
#include <QPainter>
#include <QtTest>

using namespace markshot::shot;

/// @brief 验证系统光标与软件指针共用图案，同时保留硬件缓冲区对齐约束
class CaptureCrossCursorTest final : public QObject {
    Q_OBJECT

private slots:
    /// @brief 检查硬件光标行步长与热点合法性，无参数和返回值
    void keepsAlignedHardwareBuffer()
    {
        const QCursor cursor = captureCrossCursor();
        const QImage image = cursor.pixmap().toImage().convertToFormat(QImage::Format_ARGB32);
        QVERIFY(!image.isNull());
        QCOMPARE(image.bytesPerLine() % 256, 0);
        QVERIFY(image.rect().contains(cursor.hotSpot()));
    }

    /// @brief 比较两种指针在相同热点处的完整绘制结果，无参数和返回值
    void softwarePointerMatchesHardwareCursor()
    {
        const QCursor cursor = captureCrossCursor();
        const QImage expected = cursor.pixmap().toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
        QImage actual(expected.size(), QImage::Format_ARGB32_Premultiplied);
        actual.fill(Qt::transparent);
        QPainter painter(&actual);
        drawSelectionPointer(painter, cursor.hotSpot());
        painter.end();
        QCOMPARE(actual, expected);
    }
};

QTEST_MAIN(CaptureCrossCursorTest)
#include "capture_cross_cursor_test.moc"
