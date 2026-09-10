#include "settings/settings_page_integrations.h"

#include "providers/code_scan/code_scan_provider_factory.h"
#include "providers/ocr/ocr_provider_factory.h"
#include "providers/translate/translate_provider_factory.h"
#include "settings/settings_page_plugins_model.h"
#include "settings/settings_ui_helpers.h"
#include "ui/i18n.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

namespace markshot::settings {
namespace {

QLineEdit *addSecretRow(QFormLayout *form, const QString &label, const QString &placeholder = QString())
{
    QLineEdit *edit = addTextRow(form, label, placeholder);
    edit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    return edit;
}

QWidget *createTabFormWidget(QFormLayout **outForm)
{
    auto *page = new QWidget;
    auto *form = new QFormLayout(page);
    form->setContentsMargins(12, 12, 12, 12);
    form->setSpacing(10);
    *outForm = form;
    return page;
}

}  // namespace

SettingsPageIntegrations::SettingsPageIntegrations(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = createSettingsPageLayout(this);

    // 1. Provider 状态卡片：展示各能力当前实际生效的执行方
    QFrame *providerCard = createSettingsCard(MS_TR("Provider Status"),
                                              MS_TR("Shows which provider each capability currently resolves to: "
                                                    "custom command, plugin, builtin, or the legacy helper."),
                                              this);
    QFormLayout *providerForm = settingsCardForm(providerCard);
    m_ocrProviderStatus = new QLabel(this);
    providerForm->addRow(MS_TR("OCR"), m_ocrProviderStatus);
    m_translationProviderStatus = new QLabel(this);
    providerForm->addRow(MS_TR("Translation"), m_translationProviderStatus);
    m_codeScanProviderStatus = new QLabel(this);
    providerForm->addRow(MS_TR("Code Scanner"), m_codeScanProviderStatus);
    layout->addWidget(providerCard);

    // 2. 扫码工具
    QFrame *codeCard = createSettingsCard(MS_TR("Code Scanner"),
                                          MS_TR("Configure the external helper used to recognize QR codes and barcodes."),
                                          this);
    QFormLayout *codeForm = settingsCardForm(codeCard);
    m_codeScanCommand = addTextRow(codeForm, MS_TR("Scan Command"), QStringLiteral("mark-shot-code-scan {image}"));
    m_codeScanTimeoutMs = addSpinRow(codeForm, MS_TR("Scan Timeout"), 1000, 300000, QStringLiteral(" ms"));
    layout->addWidget(codeCard);

    // 3. 上传工具
    QFrame *uploadCard = createSettingsCard(MS_TR("Image Upload"),
                                            MS_TR("Configure the external helper used to upload screenshots."),
                                            this);
    QFormLayout *uploadForm = settingsCardForm(uploadCard);
    m_uploadCommand = addTextRow(uploadForm, MS_TR("Upload Command"), QStringLiteral("mark-shot-upload {image}"));
    m_uploadTimeoutMs = addSpinRow(uploadForm, MS_TR("Upload Timeout"), 1000, 300000, QStringLiteral(" ms"));
    m_uploadEnv = addPlainTextRow(uploadForm,
                                  MS_TR("Upload Environment"),
                                  QStringLiteral("TOKEN=example"));
    layout->addWidget(uploadCard);

    // 4. OCR 与翻译集成卡片
    QFrame *translationCard = createSettingsCard(MS_TR("OCR and Translation Integration"),
                                                 MS_TR("Configure OCR and translation providers, credentials, and result panel."),
                                                 this);
    QFormLayout *translationForm = settingsCardForm(translationCard);

    // 4.1 OCR 设置
    m_ocrProvider = addComboRow(translationForm, MS_TR("OCR Provider"));
    populateProviderCombo(m_ocrProvider, providers::ProviderPluginCapability::Ocr);
    connect(m_ocrProvider, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        if (!m_updatingProviderFromSignal) {
            emit ocrProviderChanged(providerComboValue(m_ocrProvider));
        }
    });

    m_ocrResultPanel = addSwitchRow(translationForm,
                                    MS_TR("OCR Result Panel"),
                                    MS_TR("Show an editable OCR result panel before copying text."));

    // 4.2 翻译服务商选择
    m_translationProvider = addComboRow(translationForm, MS_TR("Translation Provider"));
    populateProviderCombo(m_translationProvider, providers::ProviderPluginCapability::Translation);
    connect(m_translationProvider, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        const QString value = providerComboValue(m_translationProvider);
        syncTabToProvider(value);
        if (!m_updatingProviderFromSignal) {
            emit translationProviderChanged(value);
        }
    });

    // 4.3 各厂商凭据与参数多标签页
    m_translationTabs = new QTabWidget(translationCard);

    // Tab 1: Google Gemini
    QFormLayout *geminiForm = nullptr;
    QWidget *geminiTab = createTabFormWidget(&geminiForm);
    m_cloudTranslate.geminiApiKey =
        addSecretRow(geminiForm, MS_TR("Gemini API Key"), QStringLiteral("GEMINI_API_KEY"));
    m_cloudTranslate.geminiModel =
        addTextRow(geminiForm, MS_TR("Gemini Model"), QStringLiteral("gemini-2.5-flash"));
    m_cloudTranslate.geminiEndpoint =
        addTextRow(geminiForm, MS_TR("Gemini Endpoint"), QStringLiteral("https://generativelanguage.googleapis.com"));
    m_translationTabs->addTab(geminiTab, QStringLiteral("Google Gemini"));

    // Tab 2: OpenAI 兼容
    QFormLayout *openaiForm = nullptr;
    QWidget *openaiTab = createTabFormWidget(&openaiForm);
    m_translationApiBase = addTextRow(openaiForm,
                                      MS_TR("Translation API Base"),
                                      QStringLiteral("https://api.openai.com/v1"));
    m_translationApiKey = addSecretRow(openaiForm, MS_TR("API Key"), QStringLiteral("sk-..."));
    m_translationModel = addTextRow(openaiForm, MS_TR("Translation Model"), QStringLiteral("gpt-4o-mini"));
    m_translationApiKeyEnv = addTextRow(openaiForm,
                                        MS_TR("API Key Environment"),
                                        QStringLiteral("OPENAI_API_KEY"));
    m_translationTemperature = addDoubleRow(openaiForm, MS_TR("Temperature"), 0.0, 2.0, 2);
    m_translationSystemPrompt = addPlainTextRow(openaiForm,
                                                MS_TR("System Prompt"),
                                                MS_TR("Optional translation system prompt."));
    m_translationTabs->addTab(openaiTab, MS_TR("OpenAI Compatible"));

    // Tab 3: Anthropic Claude
    QFormLayout *anthropicForm = nullptr;
    QWidget *anthropicTab = createTabFormWidget(&anthropicForm);
    m_cloudTranslate.anthropicApiKey =
        addSecretRow(anthropicForm, MS_TR("Anthropic API Key"), QStringLiteral("ANTHROPIC_API_KEY"));
    m_cloudTranslate.anthropicModel =
        addTextRow(anthropicForm, MS_TR("Anthropic Model"), QStringLiteral("claude-3-5-haiku-20241022"));
    m_cloudTranslate.anthropicEndpoint =
        addTextRow(anthropicForm, MS_TR("Anthropic Endpoint"), QStringLiteral("https://api.anthropic.com"));
    m_translationTabs->addTab(anthropicTab, QStringLiteral("Anthropic Claude"));

    // Tab 4: 国内云厂商 (腾讯 / 百度 / 有道)
    QFormLayout *cloudForm = nullptr;
    QWidget *cloudTab = createTabFormWidget(&cloudForm);
    m_cloudTranslate.tencentSecretId =
        addTextRow(cloudForm, MS_TR("Tencent SecretId"), QStringLiteral("AKID..."));
    m_cloudTranslate.tencentSecretKey =
        addSecretRow(cloudForm, MS_TR("Tencent SecretKey"), QStringLiteral("TENCENTCLOUD_SECRET_KEY"));
    m_cloudTranslate.tencentRegion =
        addTextRow(cloudForm, MS_TR("Tencent Region"), QStringLiteral("ap-guangzhou"));

    m_cloudTranslate.baiduAppId =
        addTextRow(cloudForm, MS_TR("Baidu AppID"), QStringLiteral("20150630..."));
    m_cloudTranslate.baiduAppKey =
        addSecretRow(cloudForm, MS_TR("Baidu Secret Key"), QStringLiteral("MARK_SHOT_BAIDU_APP_KEY"));

    m_cloudTranslate.youdaoAppKey =
        addTextRow(cloudForm, MS_TR("Youdao AppKey"), QStringLiteral("MARK_SHOT_YOUDAO_APP_KEY"));
    m_cloudTranslate.youdaoAppSecret =
        addSecretRow(cloudForm, MS_TR("Youdao App Secret"), QStringLiteral("MARK_SHOT_YOUDAO_APP_SECRET"));
    m_translationTabs->addTab(cloudTab, MS_TR("Cloud Services"));

    translationForm->addRow(m_translationTabs);
    layout->addWidget(translationCard);

    layout->addStretch();
}

void SettingsPageIntegrations::syncTabToProvider(const QString &providerId)
{
    if (!m_translationTabs) {
        return;
    }
    const QString norm = providerId.trimmed().toLower();
    if (norm.contains(QStringLiteral("gemini"))) {
        m_translationTabs->setCurrentIndex(0);
    } else if (norm.contains(QStringLiteral("openai")) || norm == QStringLiteral("builtin")) {
        m_translationTabs->setCurrentIndex(1);
    } else if (norm.contains(QStringLiteral("anthropic")) || norm.contains(QStringLiteral("claude"))) {
        m_translationTabs->setCurrentIndex(2);
    } else if (norm.contains(QStringLiteral("tencent")) || norm.contains(QStringLiteral("baidu"))
               || norm.contains(QStringLiteral("youdao"))) {
        m_translationTabs->setCurrentIndex(3);
    }
}

void SettingsPageIntegrations::setTranslationProvider(const QString &provider)
{
    m_updatingProviderFromSignal = true;
    setProviderComboValue(m_translationProvider, provider);
    syncTabToProvider(provider);
    m_updatingProviderFromSignal = false;
}

void SettingsPageIntegrations::setOcrProvider(const QString &provider)
{
    m_updatingProviderFromSignal = true;
    setProviderComboValue(m_ocrProvider, provider);
    m_updatingProviderFromSignal = false;
}

void SettingsPageIntegrations::setConfig(const SettingsConfig &config)
{
    m_codeScanCommand->setText(config.integrations.codeScanCommand);
    m_codeScanTimeoutMs->setValue(config.integrations.codeScanTimeoutMs);
    m_uploadCommand->setText(config.integrations.uploadCommand);
    m_uploadTimeoutMs->setValue(config.integrations.uploadTimeoutMs);
    m_uploadEnv->setPlainText(envMapToText(config.integrations.uploadEnv));

    setProviderComboValue(m_ocrProvider, config.pinned.ocrProvider);
    m_ocrResultPanel->setChecked(config.integrations.ocrResultPanelEnabled);

    setProviderComboValue(m_translationProvider, config.pinned.translationProvider);
    syncTabToProvider(config.pinned.translationProvider);

    m_translationApiBase->setText(config.integrations.translationApiBase);
    m_translationApiKeyEnv->setText(config.integrations.translationApiKeyEnv);
    m_translationApiKey->setText(config.integrations.translationApiKey);
    m_translationModel->setText(config.integrations.translationModel);
    m_translationTemperature->setValue(config.integrations.translationTemperature);
    m_translationSystemPrompt->setPlainText(config.integrations.translationSystemPrompt);

    applyCloudTranslateSettings(m_cloudTranslate, config.integrations.cloudTranslate);
    refreshProviderStatus(config);
}

void SettingsPageIntegrations::refreshProviderStatus(const SettingsConfig &config)
{
    // 1. 按 auto 链解析各能力实际生效的 provider 并展示
    markshot::providers::OcrTaskRequest ocrRequest;
    ocrRequest.provider = config.pinned.ocrProvider;
    ocrRequest.commandLine = config.pinned.ocrCommand.trimmed();
    ocrRequest.backend = config.pinned.ocrBackend;
    m_ocrProviderStatus->setText(markshot::providers::resolvedOcrProviderName(ocrRequest));

    markshot::providers::TranslateTaskRequest translateRequest;
    translateRequest.provider = config.pinned.translationProvider;
    translateRequest.commandLine = config.pinned.translationCommand.trimmed();
    m_translationProviderStatus->setText(
        markshot::providers::resolvedTranslateProviderName(translateRequest));

    markshot::providers::CodeScanTaskRequest codeScanRequest;
    codeScanRequest.provider = config.integrations.codeScanProvider;
    codeScanRequest.commandLine = config.integrations.codeScanCommand.trimmed();
    m_codeScanProviderStatus->setText(
        markshot::providers::resolvedCodeScanProviderName(codeScanRequest));
}

void SettingsPageIntegrations::updateConfig(SettingsConfig *config) const
{
    if (!config) {
        return;
    }

    config->integrations.codeScanCommand = m_codeScanCommand->text().trimmed();
    config->integrations.codeScanTimeoutMs = m_codeScanTimeoutMs->value();
    config->integrations.uploadCommand = m_uploadCommand->text().trimmed();
    config->integrations.uploadTimeoutMs = m_uploadTimeoutMs->value();
    config->integrations.uploadEnv = envMapFromText(m_uploadEnv->toPlainText());

    config->pinned.ocrProvider = providerComboValue(m_ocrProvider);
    config->integrations.ocrResultPanelEnabled = m_ocrResultPanel->isChecked();

    config->pinned.translationProvider = providerComboValue(m_translationProvider);

    config->integrations.translationApiBase = m_translationApiBase->text().trimmed();
    config->integrations.translationApiKeyEnv = m_translationApiKeyEnv->text().trimmed();
    config->integrations.translationApiKey = m_translationApiKey->text().trimmed();
    config->integrations.translationModel = m_translationModel->text().trimmed();
    config->integrations.translationTemperature = m_translationTemperature->value();
    config->integrations.translationSystemPrompt = m_translationSystemPrompt->toPlainText();

    collectCloudTranslateSettings(m_cloudTranslate, &config->integrations.cloudTranslate);
}

}  // namespace markshot::settings
