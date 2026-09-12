#include "settings/settings_dialog.h"

#include "app_config_store.h"
#include "settings/settings_design_tokens.h"
#include "settings/settings_navigation.h"
#include "settings/settings_page_about.h"
#include "settings/settings_page_advanced.h"
#include "settings/settings_page_annotation.h"
#include "settings/settings_page_capture.h"
#include "settings/settings_page_general.h"
#include "settings/settings_page_integrations.h"
#include "settings/settings_page_pinned.h"
#include "settings/settings_page_plugins.h"
#include "settings/settings_page_scroll.h"
#include "settings/settings_page_shortcuts.h"
#include "settings/settings_page_storage.h"
#include "settings/settings_wheel_guard.h"
#include "ui/i18n.h"
#include "ui/application_icon.h"
#include "ui/icons.h"
#include "ui/theme.h"
#include "ui/interface_theme_config.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QResizeEvent>
#include <QScopedValueRollback>
#include <QPointer>
#include <QPushButton>
#include <QScreen>
#include <QGuiApplication>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStyle>
#include <QVBoxLayout>

namespace markshot::settings {
namespace {

/// @brief 将设置页包装成可滚动页面。
/// @param stack 目标堆叠控件。
/// @param page 需要显示的设置页。
void addScrollablePage(QStackedWidget *stack, QWidget *page)
{
    auto *area = new QScrollArea(stack);
    area->setFrameShape(QFrame::NoFrame);
    area->setWidgetResizable(true);
    area->setWidget(page);
    stack->addWidget(area);
}

/**
 * 读取配置中的设置界面主题。
 * @return 配置的界面主题模式。
 */
markshot::ui::UiThemeMode configuredSettingsThemeMode()
{
    bool ok = false;
    const QJsonObject root = markshot::readAppConfigRoot(&ok);
    return ok ? markshot::ui::uiThemeModeFromConfigRoot(root)
              : markshot::ui::UiThemeMode::System;
}

}  // namespace

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("settingsDialog"));
    setWindowTitle(MS_TR("Settings"));
    setWindowIcon(markshot::ui::applicationIcon());
    setFont(markshot::theme::uiFont(10));
    setMinimumSize(420, 400);
    resize(860, 620);

    applyTheme(configuredSettingsThemeMode());

    // 滚轮防护：未聚焦的下拉框/数值框不再被滚轮误改内容，页面照常滚动。
    installSettingsWheelGuard(this);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *body = new QWidget(this);
    auto *bodyLayout = new QHBoxLayout(body);
    m_bodyLayout = bodyLayout;
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    // 侧栏导航：标题区 + 分组分类列表
    m_navigation = new SettingsNavigation(body);
    bodyLayout->addWidget(m_navigation);

    // 内容栈：11 个可滚动设置页
    m_stack = new QStackedWidget(body);
    m_generalPage = new SettingsPageGeneral(m_stack);
    m_capturePage = new SettingsPageCapture(m_stack);
    m_shortcutsPage = new SettingsPageShortcuts(m_stack);
    m_annotationPage = new SettingsPageAnnotation(m_stack);
    m_pinnedPage = new SettingsPagePinned(m_stack);
    m_integrationsPage = new SettingsPageIntegrations(m_stack);
    m_pluginsPage = new SettingsPagePlugins(m_stack);
    m_scrollPage = new SettingsPageScroll(m_stack);
    m_storagePage = new SettingsPageStorage(m_stack);
    m_advancedPage = new SettingsPageAdvanced(m_stack);
    m_aboutPage = new SettingsPageAbout(m_stack);
    addScrollablePage(m_stack, m_generalPage);
    addScrollablePage(m_stack, m_capturePage);
    addScrollablePage(m_stack, m_shortcutsPage);
    addScrollablePage(m_stack, m_annotationPage);
    addScrollablePage(m_stack, m_pinnedPage);
    addScrollablePage(m_stack, m_integrationsPage);
    addScrollablePage(m_stack, m_pluginsPage);
    addScrollablePage(m_stack, m_scrollPage);
    addScrollablePage(m_stack, m_storagePage);
    addScrollablePage(m_stack, m_advancedPage);
    addScrollablePage(m_stack, m_aboutPage);
    bodyLayout->addWidget(m_stack, 1);
    rootLayout->addWidget(body, 1);

    auto *footer = new QFrame(this);
    footer->setObjectName(QStringLiteral("settingsFooter"));
    auto *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(16, 10, 16, 10);
    footerLayout->setSpacing(8);
    m_revertButton = new QPushButton(MS_TR("Undo changes"), footer);
    m_revertButton->setObjectName(QStringLiteral("settingsRevert"));
    m_revertButton->setProperty("role", QStringLiteral("quiet"));
    m_revertButton->setAutoDefault(false);
    m_revertButton->setEnabled(false);
    footerLayout->addWidget(m_revertButton);
    m_statusLabel = new QLabel(footer);
    m_statusLabel->setObjectName(QStringLiteral("settingsStatus"));
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    footerLayout->addWidget(m_statusLabel, 1);
    auto *closeButton = new QPushButton(MS_TR("Close"), footer);
    closeButton->setProperty("role", QStringLiteral("quiet"));
    closeButton->setAutoDefault(false);
    footerLayout->addWidget(closeButton);
    m_saveButton = new QPushButton(MS_TR("Save"), footer);
    m_saveButton->setObjectName(QStringLiteral("settingsSave"));
    m_saveButton->setProperty("role", QStringLiteral("primary"));
    // 1. 【设置】【键盘保存】输入框中的 Enter 只执行保存，避免触发关闭或撤销
    m_saveButton->setDefault(true);
    m_saveButton->setEnabled(false);
    footerLayout->addWidget(m_saveButton);
    rootLayout->addWidget(footer);

    // 1. 【设置】【导航】切换分类时保留各页尚未保存的输入
    connect(m_navigation, &SettingsNavigation::navigationChanged, m_stack, &QStackedWidget::setCurrentIndex);
    // 2. 【设置】【保存与撤销】反馈保留在操作栏，撤销回到最近一次保存的配置
    connect(m_saveButton, &QPushButton::clicked, this, &SettingsDialog::saveConfig);
    connect(m_revertButton, &QPushButton::clicked, this, &SettingsDialog::revertChanges);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);

    m_navigation->setCurrentLogicalRow(0);
    loadConfig();
    trackSettingChanges();
}

void SettingsDialog::loadConfig()
{
    QString error;
    m_config = readSettingsConfig(&error);
    applyConfigToPages(m_config);
    if (!error.isEmpty()) {
        setStatus(error, true);
    }
    applyTheme(m_config.general.uiThemeMode);
}

void SettingsDialog::applyConfigToPages(const SettingsConfig &config)
{
    const QScopedValueRollback<bool> applying(m_applyingConfig, true);
    m_generalPage->setConfig(config);
    m_capturePage->setConfig(config);
    m_shortcutsPage->setConfig(config);
    m_annotationPage->setConfig(config);
    m_pinnedPage->setConfig(config);
    m_integrationsPage->setConfig(config);
    m_pluginsPage->setConfig(config);
    m_scrollPage->setConfig(config);
    m_storagePage->setConfig(config);
    m_advancedPage->setConfig(config);
    m_savedValues = settingsConfigToJson(collectConfig());
    m_revertButton->setEnabled(false);
    m_saveButton->setEnabled(false);
}

SettingsConfig SettingsDialog::collectConfig() const
{
    SettingsConfig config = m_config;
    m_generalPage->updateConfig(&config);
    m_capturePage->updateConfig(&config);
    m_shortcutsPage->updateConfig(&config);
    m_annotationPage->updateConfig(&config);
    m_pinnedPage->updateConfig(&config);
    m_integrationsPage->updateConfig(&config);
    m_pluginsPage->updateConfig(&config);
    m_scrollPage->updateConfig(&config);
    m_storagePage->updateConfig(&config);
    m_advancedPage->updateConfig(&config);
    return config;
}

void SettingsDialog::saveConfig()
{
    SettingsConfig nextConfig = collectConfig();
    QString error;
    if (!writeSettingsConfig(nextConfig, &error)) {
        setStatus(MS_TR("Failed to save settings: %1").arg(error), true);
        return;
    }

    m_config = nextConfig;
    // 1. 【设置】【保存基线】撤销修改恢复到刚保存的值
    applyConfigToPages(nextConfig);
    applyTheme(m_config.general.uiThemeMode);
    setStatus(MS_TR("Changes saved"));
    m_statusLabel->setToolTip(MS_TR("Some changes take effect after restarting Mark Shot."));
}

void SettingsDialog::applyTheme(markshot::ui::UiThemeMode mode)
{
    const markshot::ui::UiThemeMode effectiveMode = markshot::ui::effectiveUiThemeMode(mode);
    qApp->setPalette(tokens::settingsPalette(effectiveMode));
    setStyleSheet(tokens::settingsStyleSheet(effectiveMode));
}

void SettingsDialog::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    if (m_navigation && m_bodyLayout) {
        const bool compact = width() < 720;
        m_navigation->setCompact(compact);
        m_bodyLayout->setDirection(compact ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
    }
}

void SettingsDialog::trackSettingChanges()
{
    for (QLineEdit *edit : m_stack->findChildren<QLineEdit *>()) {
        if (!edit->isReadOnly()) {
            connect(edit, &QLineEdit::textChanged, this, &SettingsDialog::markSettingsChanged);
        }
    }
    for (QPlainTextEdit *edit : m_stack->findChildren<QPlainTextEdit *>()) {
        if (!edit->isReadOnly()) {
            connect(edit, &QPlainTextEdit::textChanged, this, &SettingsDialog::markSettingsChanged);
        }
    }
    for (QComboBox *combo : m_stack->findChildren<QComboBox *>()) {
        connect(combo, &QComboBox::currentIndexChanged, this, &SettingsDialog::markSettingsChanged);
    }
    for (QCheckBox *box : m_stack->findChildren<QCheckBox *>()) {
        connect(box, &QCheckBox::toggled, this, &SettingsDialog::markSettingsChanged);
    }
    for (QSpinBox *spin : m_stack->findChildren<QSpinBox *>()) {
        connect(spin, &QSpinBox::valueChanged, this, &SettingsDialog::markSettingsChanged);
    }
    for (QDoubleSpinBox *spin : m_stack->findChildren<QDoubleSpinBox *>()) {
        connect(spin, &QDoubleSpinBox::valueChanged, this, &SettingsDialog::markSettingsChanged);
    }
    for (QKeySequenceEdit *edit : m_stack->findChildren<QKeySequenceEdit *>()) {
        connect(edit, &QKeySequenceEdit::keySequenceChanged, this, &SettingsDialog::markSettingsChanged);
    }
    for (QPushButton *button : m_stack->findChildren<QPushButton *>()) {
        connect(button, &QPushButton::clicked, this, &SettingsDialog::markSettingsChanged);
    }
}

void SettingsDialog::markSettingsChanged()
{
    if (m_applyingConfig) {
        return;
    }
    // 1. 【设置】【变更反馈】仅比较可保存的配置值，排除下载和按钮文案等独立状态
    const bool wasModified = m_saveButton->isEnabled();
    const bool modified = settingsConfigToJson(collectConfig()) != m_savedValues;
    m_revertButton->setEnabled(modified);
    m_saveButton->setEnabled(modified);
    if (modified) {
        setStatus(MS_TR("Unsaved changes"));
    } else if (wasModified) {
        setStatus({});
    }
}

void SettingsDialog::setStatus(const QString &text, bool error)
{
    m_statusLabel->setText(text);
    m_statusLabel->setToolTip(text);
    m_statusLabel->setProperty("tone", error ? QStringLiteral("error") : QString());
    m_statusLabel->style()->unpolish(m_statusLabel);
    m_statusLabel->style()->polish(m_statusLabel);
}

void SettingsDialog::revertChanges()
{
    applyConfigToPages(m_config);
    applyTheme(m_config.general.uiThemeMode);
    setStatus(MS_TR("Changes reverted"));
}

void showSettingsDialog(QWidget *parent)
{
    static QPointer<SettingsDialog> dialog;
    if (!dialog) {
        dialog = new SettingsDialog(nullptr);
        dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    }

    if (parent && parent->screen()) {
        const QRect available = parent->screen()->availableGeometry();
        dialog->move(available.center() - dialog->rect().center());
    } else if (QScreen *screen = QGuiApplication::primaryScreen()) {
        const QRect available = screen->availableGeometry();
        dialog->move(available.center() - dialog->rect().center());
    }

    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

}  // namespace markshot::settings
