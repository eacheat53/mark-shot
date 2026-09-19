#include "app_config_store.h"
#include "pinned_window/pinned_image_window.h"
#include "shell_command.h"
#include "window_detection.h"

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QMouseEvent>
#include <QTemporaryDir>
#include <QTextStream>
#include <QtTest>

namespace {

/**
 * 【贴图测试】【鼠标输入】向真实窗口发送包含完整按键状态的事件
 * @param window 目标窗口
 * @param type 事件类型
 * @param point 窗口内坐标
 * @param button 本次改变的按钮
 * @param buttons 当前按住的按钮
 * @return 无返回值
 */
void pointerEvent(QWidget &window, QEvent::Type type, QPoint point,
                  Qt::MouseButton button = Qt::NoButton, Qt::MouseButtons buttons = Qt::NoButton)
{
    QMouseEvent event(type, QPointF(point), QPointF(window.mapToGlobal(point)),
                      button, buttons, Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &event);
}

}

class PinnedTextSelectionTest final : public QObject {
    Q_OBJECT
private slots:
    /// @brief 提供普通屏幕和高分屏的首次拖选场景，无参数和返回值
    void selectsTextWithoutAutomaticOcr_data()
    {
        QTest::addColumn<qreal>("dpr");
        QTest::newRow("normal") << 1.0;
        QTest::newRow("hidpi") << 2.0;
    }

    /**
     * 【贴图测试】【按需识别】关闭自动识别后仍可拖选文字，且不会移动图片
     * @return 无返回值
     */
    void selectsTextWithoutAutomaticOcr()
    {
        QFETCH(qreal, dpr);
        const QString command = markshot::shot::shellQuote(QCoreApplication::applicationFilePath())
            + QStringLiteral(" --ocr-fixture");
        QVERIFY(markshot::writeAppConfigRoot({
            {QStringLiteral("ocr"), QJsonObject{{QStringLiteral("enabled"), true},
                                                {QStringLiteral("command"), command}}},
            {QStringLiteral("pinnedWindow"), QJsonObject{{QStringLiteral("autoOcr"), false},
                {QStringLiteral("alwaysOnTop"), false}, {QStringLiteral("textSelectionCopyEnabled"), true}}},
            {QStringLiteral("translation"), QJsonObject{{QStringLiteral("autoAfterOcr"), false}}}
        }));
        QImage image(400, 200, QImage::Format_RGB32);
        image.setDevicePixelRatio(dpr);
        image.fill(Qt::white);
        markshot::shot::PinnedImageWindow window(image, QPoint(80, 80));
        window.setAttribute(Qt::WA_DeleteOnClose, false);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const QPoint position = window.pos();
        // 1. 【贴图测试】【拖选】首次手势发生在尚未执行 OCR 的窗口
        pointerEvent(window, QEvent::MouseButtonPress, QPoint(55 / dpr, 60 / dpr), Qt::LeftButton, Qt::LeftButton);
        pointerEvent(window, QEvent::MouseMove, QPoint(150 / dpr, 60 / dpr), Qt::NoButton, Qt::LeftButton);
        pointerEvent(window, QEvent::MouseButtonRelease, QPoint(150 / dpr, 60 / dpr), Qt::LeftButton);
        // 2. 【贴图测试】【手势边界】释放后的指针移动不能改变等待中的选区
        pointerEvent(window, QEvent::MouseMove, QPoint(55 / dpr, 60 / dpr));
        QCOMPARE(window.pos(), position);
        // 3. 【贴图测试】【复制】等待异步识别后通过实际快捷键检查选区文字
        QApplication::clipboard()->clear();
        QTRY_VERIFY_WITH_TIMEOUT(([&] {
            QTest::keyClick(&window, Qt::Key_C, Qt::ControlModifier);
            return QApplication::clipboard()->text() == QStringLiteral("Hello world");
        })(), 3000);
    }

    /// @brief 检查首次识别时不同拖动起点和开关组合，无参数和返回值
    void preservesImageMovement_data()
    {
        QTest::addColumn<bool>("selectionEnabled");
        QTest::addColumn<bool>("ocrEnabled");
        QTest::addColumn<bool>("emptyResult");
        QTest::newRow("blank-area") << true << true << false;
        QTest::newRow("no-text") << true << true << true;
        QTest::newRow("selection-disabled") << false << true << false;
        QTest::newRow("ocr-disabled") << true << false << false;
    }

    /// @brief 没有可选文字时保留图片移动，避免按需识别阻断原有交互，无参数和返回值
    void preservesImageMovement()
    {
        QFETCH(bool, selectionEnabled);
        QFETCH(bool, ocrEnabled);
        QFETCH(bool, emptyResult);
        const QString command = markshot::shot::shellQuote(QCoreApplication::applicationFilePath())
            + (emptyResult ? QStringLiteral(" --ocr-empty") : QStringLiteral(" --ocr-fixture"));
        QVERIFY(markshot::writeAppConfigRoot({
            {QStringLiteral("ocr"), QJsonObject{{QStringLiteral("enabled"), ocrEnabled},
                                                {QStringLiteral("command"), command}}},
            {QStringLiteral("pinnedWindow"), QJsonObject{{QStringLiteral("autoOcr"), false},
                {QStringLiteral("alwaysOnTop"), false}, {QStringLiteral("textSelectionCopyEnabled"), selectionEnabled}}},
            {QStringLiteral("translation"), QJsonObject{{QStringLiteral("autoAfterOcr"), false}}}
        }));
        QImage image(400, 200, QImage::Format_RGB32);
        image.fill(Qt::white);
        markshot::shot::PinnedImageWindow window(image, QPoint(80, 80));
        window.setAttribute(Qt::WA_DeleteOnClose, false);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const QPoint position = window.pos();
        pointerEvent(window, QEvent::MouseButtonPress, QPoint(250, 150), Qt::LeftButton, Qt::LeftButton);
        pointerEvent(window, QEvent::MouseMove, QPoint(290, 150), Qt::NoButton, Qt::LeftButton);
        pointerEvent(window, QEvent::MouseButtonRelease, QPoint(290, 150), Qt::LeftButton);
        QTRY_COMPARE_WITH_TIMEOUT(window.pos(), position + QPoint(40, 0), 3000);
    }
};

/**
 * 【贴图测试】【隔离环境】运行窗口测试或输出受控 OCR 结果
 * @param argc 参数数量
 * @param argv 参数列表
 * @return 测试退出码
 */
int main(int argc, char **argv)
{
    if (argc > 1 && (QByteArray(argv[1]) == "--ocr-fixture" || QByteArray(argv[1]) == "--ocr-empty")) {
        QTextStream(stdout) << (QByteArray(argv[1]) == "--ocr-empty" ? "[]"
            : R"({"tokens":[{"text":"Hello","box":[40,40,60,40],"line":0,"index":0},{"text":"world","box":[120,40,60,40],"line":0,"index":1}]})");
        return 0;
    }
    QTemporaryDir isolated;
    if (!isolated.isValid()) {
        return 1;
    }
    qputenv("XDG_CONFIG_HOME", isolated.path().toUtf8());
    qputenv("XDG_DATA_HOME", isolated.path().toUtf8());
    qputenv("XDG_SESSION_TYPE", "offscreen");
    qputenv("LOCALAPPDATA", isolated.path().toUtf8());
    qputenv("APPDATA", isolated.path().toUtf8());
    // 1. 【贴图测试】【配置隔离】预先创建配置，阻止读取器回退到真实用户目录
    QDir().mkpath(isolated.filePath(QStringLiteral("mark-shot")));
    QFile config(isolated.filePath(QStringLiteral("mark-shot/config.json")));
    if (!config.open(QIODevice::WriteOnly) || config.write("{}") != 2) {
        return 1;
    }
    config.close();
    qunsetenv("DISPLAY");
    qunsetenv("MARK_SHOT_OCR_COMMAND");
    qunsetenv("MARK_SHOT_PINNED_AUTO_OCR");
    qunsetenv("MARK_SHOT_OCR_DISABLED");
    qunsetenv("MARK_SHOT_TRANSLATION_AUTO_AFTER_OCR");
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("mark-shot-selection-test"));
    if (!markshot::appConfigPath().startsWith(isolated.path() + QLatin1Char('/'))) {
        return 1;
    }
    PinnedTextSelectionTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "pinned_text_selection_test.moc"
