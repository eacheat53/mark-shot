#include "settings/settings_page_plugins.h"

#include "providers/provider_plugin_info.h"
#include "settings/settings_page_plugins_model.h"
#include "settings/settings_ui_helpers.h"
#include "ui/i18n.h"

#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QLayoutItem>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QUrl>
#include <QVBoxLayout>

namespace markshot::settings {
namespace {

/**
 * 读取非空展示文本。
 * @param text 原始文本。
 * @param fallback 兜底文本。
 * @return 去除首尾空白后的展示文本。
 */
QString displayText(const QString &text, const QString &fallback)
{
    const QString trimmed = text.trimmed();
    return trimmed.isEmpty() ? fallback : trimmed;
}

/**
 * 读取插件状态对应的视觉语义。
 * @param status 插件状态文本。
 * @return 状态语义标识。
 */
QString statusTone(const QString &status)
{
    if (status == MS_TR("Available")) {
        return QStringLiteral("success");
    }
    if (status == MS_TR("Load failed") || status == MS_TR("Unavailable")) {
        return QStringLiteral("error");
    }
    return QStringLiteral("muted");
}

/**
 * 创建可换行的只读文本标签。
 * @param text 标签文本。
 * @param objectName 对象名，用于主题样式。
 * @param parent 父控件。
 * @return 文本标签。
 */
QLabel *createWrapLabel(const QString &text, const QString &objectName, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(objectName);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    return label;
}

/**
 * 创建诊断字段块。
 * @param title 字段标题。
 * @param value 字段值。
 * @param parent 父控件。
 * @return 字段块控件。
 */
QWidget *createDiagnosticField(const QString &title, const QString &value, QWidget *parent)
{
    auto *field = new QWidget(parent);
    auto *layout = new QVBoxLayout(field);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    auto *titleLabel = new QLabel(title, field);
    titleLabel->setObjectName(QStringLiteral("pluginDiagnosticFieldTitle"));

    QLabel *valueLabel = createWrapLabel(value, QStringLiteral("pluginDiagnosticFieldValue"), field);
    layout->addWidget(titleLabel);
    layout->addWidget(valueLabel);
    return field;
}

/**
 * 创建单条插件诊断卡片。
 * @param data 插件诊断行数据。
 * @param parent 父控件。
 * @return 诊断卡片控件。
 */
QFrame *createDiagnosticItem(const PluginDiagnosticRow &data, QWidget *parent)
{
    auto *item = new QFrame(parent);
    item->setObjectName(QStringLiteral("pluginDiagnosticItem"));
    auto *layout = new QVBoxLayout(item);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(8);

    auto *summaryLayout = new QHBoxLayout;
    summaryLayout->setContentsMargins(0, 0, 0, 0);
    summaryLayout->setSpacing(10);

    auto *identityLayout = new QVBoxLayout;
    identityLayout->setContentsMargins(0, 0, 0, 0);
    identityLayout->setSpacing(2);

    QLabel *provider = createWrapLabel(displayText(data.provider, MS_TR("Plugin")),
                                       QStringLiteral("pluginDiagnosticProvider"),
                                       item);
    auto *capability = new QLabel(data.capability, item);
    capability->setObjectName(QStringLiteral("pluginDiagnosticMeta"));

    auto *status = new QLabel(displayText(data.status, MS_TR("Unknown")), item);
    status->setObjectName(QStringLiteral("pluginDiagnosticStatus"));
    status->setProperty("tone", statusTone(data.status));
    status->setAlignment(Qt::AlignCenter);
    status->setMinimumHeight(24);

    identityLayout->addWidget(provider);
    identityLayout->addWidget(capability);
    summaryLayout->addLayout(identityLayout, 1);
    summaryLayout->addWidget(status, 0, Qt::AlignTop);
    layout->addLayout(summaryLayout);

    if (!data.path.trimmed().isEmpty()) {
        layout->addWidget(createDiagnosticField(MS_TR("Path"), data.path, item));
    }
    if (!data.details.trimmed().isEmpty()) {
        layout->addWidget(createDiagnosticField(MS_TR("Details"), data.details, item));
    }
    return item;
}

/**
 * 清空布局中的全部条目。
 * @param layout 需要清空的布局。
 * @return 无返回值。
 */
void clearLayout(QLayout *layout)
{
    if (!layout) {
        return;
    }
    while (QLayoutItem *item = layout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
}

}  // namespace

SettingsPagePlugins::SettingsPagePlugins(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = createSettingsPageLayout(this);
    buildProviderCard(layout);
    buildMarketplaceCard(layout);
    buildOcrModelCard(layout);
    buildDirectoriesCard(layout);
    buildDiagnosticsCard(layout);
    layout->addStretch();
}

void SettingsPagePlugins::setConfig(const SettingsConfig &config)
{
    setProviderComboValue(m_ocrProvider, config.pinned.ocrProvider);
    setProviderComboValue(m_translationProvider, config.pinned.translationProvider);
    setProviderComboValue(m_codeScanProvider, config.integrations.codeScanProvider);
    refreshDiagnostics();
}

void SettingsPagePlugins::updateConfig(SettingsConfig *config) const
{
    if (!config) {
        return;
    }
    config->pinned.ocrProvider = providerComboValue(m_ocrProvider);
    config->pinned.translationProvider = providerComboValue(m_translationProvider);
    config->integrations.codeScanProvider = providerComboValue(m_codeScanProvider);
}

void SettingsPagePlugins::setTranslationProvider(const QString &provider)
{
    m_updatingProviderFromSignal = true;
    setProviderComboValue(m_translationProvider, provider);
    m_updatingProviderFromSignal = false;
}

void SettingsPagePlugins::setOcrProvider(const QString &provider)
{
    m_updatingProviderFromSignal = true;
    setProviderComboValue(m_ocrProvider, provider);
    m_updatingProviderFromSignal = false;
}

void SettingsPagePlugins::buildProviderCard(QVBoxLayout *layout)
{
    QFrame *card = createSettingsCard(MS_TR("Plugin Providers"),
                                      MS_TR("Choose which provider Mark Shot uses for OCR, translation, and code scanning."),
                                      this);
    QFormLayout *form = settingsCardForm(card);
    m_ocrProvider = addComboRow(form, MS_TR("OCR Provider"));
    m_translationProvider = addComboRow(form, MS_TR("Translation Provider"));
    m_codeScanProvider = addComboRow(form, MS_TR("Code Scanner Provider"));

    populateProviderCombo(m_ocrProvider, providers::ProviderPluginCapability::Ocr);
    populateProviderCombo(m_translationProvider, providers::ProviderPluginCapability::Translation);
    populateProviderCombo(m_codeScanProvider, providers::ProviderPluginCapability::CodeScan);

    connect(m_ocrProvider, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        if (!m_updatingProviderFromSignal) {
            emit ocrProviderChanged(providerComboValue(m_ocrProvider));
        }
    });
    connect(m_translationProvider, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        if (!m_updatingProviderFromSignal) {
            emit translationProviderChanged(providerComboValue(m_translationProvider));
        }
    });

    layout->addWidget(card);
}

void SettingsPagePlugins::buildDirectoriesCard(QVBoxLayout *layout)
{
    QFrame *card = createSettingsCard(MS_TR("Plugin Directories"),
                                      MS_TR("Provider plugins are loaded from these directories when Mark Shot starts."),
                                      this);
    QFormLayout *form = settingsCardForm(card);
    m_directories = new QPlainTextEdit(card);
    m_directories->setReadOnly(true);
    m_directories->setMinimumHeight(92);
    m_directories->setPlainText(pluginSearchDirectoryRows().join(QStringLiteral("\n")));
    form->addRow(MS_TR("Search Directories"), m_directories);

    m_openUserDirectory = new QPushButton(MS_TR("Open User Plugin Folder"), card);
    m_openUserDirectory->setCursor(Qt::PointingHandCursor);
    connect(m_openUserDirectory, &QPushButton::clicked, this, [] {
        const QString path = userPluginDirectory();
        QDir().mkpath(path);
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });
    form->addRow(QString(), m_openUserDirectory);
    layout->addWidget(card);
}

void SettingsPagePlugins::buildDiagnosticsCard(QVBoxLayout *layout)
{
    QFrame *card = createSettingsCard(MS_TR("Plugin Diagnostics"),
                                      MS_TR("Inspect loaded provider plugins and reasons for unavailable plugins."),
                                      this);
    QFormLayout *form = settingsCardForm(card);

    // 诊断条目直接放进卡片，由设置页外层滚动区统一滚动。此前这里内嵌了
    // 一个固定高度的 QScrollArea，悬停其上时滚轮事件被内层吃掉，外层页面
    // 无法继续滚动。
    m_diagnosticsContainer = new QWidget(card);
    m_diagnosticsContainer->setObjectName(QStringLiteral("pluginDiagnosticsViewport"));
    m_diagnosticsLayout = new QVBoxLayout(m_diagnosticsContainer);
    m_diagnosticsLayout->setContentsMargins(0, 0, 0, 0);
    m_diagnosticsLayout->setSpacing(8);

    form->addRow(m_diagnosticsContainer);
    refreshDiagnostics();
    layout->addWidget(card);
}

void SettingsPagePlugins::refreshDiagnostics()
{
    if (!m_diagnosticsLayout) {
        return;
    }

    clearLayout(m_diagnosticsLayout);
    const QVector<PluginDiagnosticRow> rows = pluginDiagnosticRows();
    if (rows.isEmpty()) {
        QLabel *empty = createWrapLabel(MS_TR("No provider plugins were found in the configured search directories."),
                                        QStringLiteral("pluginDiagnosticEmpty"),
                                        m_diagnosticsContainer);
        m_diagnosticsLayout->addWidget(empty);
        m_diagnosticsLayout->addStretch();
        return;
    }

    for (const PluginDiagnosticRow &data : rows) {
        m_diagnosticsLayout->addWidget(createDiagnosticItem(data, m_diagnosticsContainer));
    }
    m_diagnosticsLayout->addStretch();
}

}  // namespace markshot::settings
