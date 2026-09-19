#include "rapid_model_paths.h"
#include "markshot/ocr_provider_plugin.h"
#include "marketplace/plugin_installer.h"

#include <QtTest/QtTest>

#include <QFont>
#include <QImage>
#include <QPainter>
#include <QPluginLoader>
#include <QTemporaryDir>

using namespace markshot::ocr_rapid;

namespace {

/**
 * 渲染一张黑字白底的文本图像。
 * @param text 需要渲染的文本。
 * @return 渲染后的图像。
 */
QImage renderTextImage(const QString &text)
{
    QImage image(480, 120, QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setPen(Qt::black);
    QFont font = painter.font();
    font.setPixelSize(42);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(image.rect(), Qt::AlignCenter, text);
    painter.end();
    return image;
}

}  // namespace

class OcrRapidPluginTest : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_pluginDirectory;
    QPluginLoader m_loader;
    markshot::plugin::OcrProviderPlugin *m_plugin = nullptr;

private slots:
    /**
     * 【OCR】【插件加载】安装生产动态库并验证插件导出接口和运行时依赖
     * @return 无返回值，加载失败时立即报告具体错误
     */
    void initTestCase()
    {
        const QString overridePath = qEnvironmentVariable("MARK_SHOT_TEST_OCR_PLUGIN_PATH");
        const QString sourcePath = overridePath.isEmpty()
            ? QString::fromUtf8(MARK_SHOT_TEST_OCR_PLUGIN_PATH) : overridePath;
        // 1. 【OCR】【插件加载】使用市场安装器复制到独立目录，验证可分发的真实库文件
        QVERIFY(m_pluginDirectory.isValid());
        const auto installed = markshot::marketplace::installPluginAsset(
            {sourcePath, QString(), m_pluginDirectory.path(), QString()});
        QVERIFY2(installed.success, qPrintable(installed.error));
        // 2. 【OCR】【插件加载】从安装结果加载，不预先链接插件或 ONNX Runtime
        m_loader.setFileName(installed.installedPath);
        m_loader.setLoadHints(QLibrary::ResolveAllSymbolsHint);
        QObject *instance = m_loader.instance();
        QVERIFY2(instance, qPrintable(m_loader.errorString()));
        m_plugin = qobject_cast<markshot::plugin::OcrProviderPlugin *>(instance);
        QVERIFY2(m_plugin, "OCR provider interface is missing");
        QCOMPARE(m_plugin->providerId(), QStringLiteral("rapid-onnx"));
    }

    /**
     * 【OCR】【插件清理】卸载动态库，使 Windows 也能删除测试安装目录
     * @return 无返回值
     */
    void cleanupTestCase()
    {
        m_plugin = nullptr;
        if (m_loader.isLoaded()) {
            QVERIFY2(m_loader.unload(), qPrintable(m_loader.errorString()));
        }
    }

    /**
     * 验证插件可用真实 PP-OCR 模型识别渲染文本。
     * @return 无返回值。
     */
    void recognizesRenderedText()
    {
        if (!locateRapidModels().isComplete()) {
            QSKIP("PP-OCR models are not available on this machine");
        }

        QString error;
        QVERIFY2(m_plugin->isAvailable(&error), qPrintable(error));

        QVector<markshot::plugin::OcrToken> tokens;
        QVERIFY2(m_plugin->recognize(renderTextImage(QStringLiteral("HELLO 123")), &tokens, &error),
                 qPrintable(error));
        QVERIFY(!tokens.isEmpty());

        QString combined;
        for (const markshot::plugin::OcrToken &token : tokens) {
            combined += token.text;
            combined += QLatin1Char(' ');
        }
        QVERIFY2(combined.contains(QStringLiteral("HELLO"), Qt::CaseInsensitive),
                 qPrintable(QStringLiteral("recognized: %1").arg(combined)));
        QVERIFY2(combined.contains(QStringLiteral("123")),
                 qPrintable(QStringLiteral("recognized: %1").arg(combined)));
    }

    /**
     * 验证中文渲染文本识别。
     * @return 无返回值。
     */
    void recognizesChineseText()
    {
        if (!locateRapidModels().isComplete()) {
            QSKIP("PP-OCR models are not available on this machine");
        }

        QString error;
        QVector<markshot::plugin::OcrToken> tokens;
        QVERIFY2(m_plugin->recognize(renderTextImage(QStringLiteral("你好世界")), &tokens, &error),
                 qPrintable(error));

        QString combined;
        for (const markshot::plugin::OcrToken &token : tokens) {
            combined += token.text;
        }
        QVERIFY2(combined.contains(QStringLiteral("你好")),
                 qPrintable(QStringLiteral("recognized: %1").arg(combined)));
    }
};

QTEST_MAIN(OcrRapidPluginTest)
#include "ocr_rapid_plugin_test.moc"
