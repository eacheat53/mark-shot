#pragma once

#include "settings/settings_cloud_translate_card.h"
#include "settings/settings_config.h"

#include <QWidget>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;
class QTabWidget;

namespace markshot::settings {

class SettingsPageIntegrations final : public QWidget {
    Q_OBJECT

public:
    /// @brief 创建外部集成设置页。
    /// @param parent 父控件。
    explicit SettingsPageIntegrations(QWidget *parent = nullptr);

    /// @brief 将配置加载到页面控件。
    /// @param config 设置配置。
    void setConfig(const SettingsConfig &config);

    /// @brief 将页面控件值写回配置。
    /// @param config 需要更新的设置配置。
    void updateConfig(SettingsConfig *config) const;

signals:
    /// @brief 翻译提供方变更信号。
    void translationProviderChanged(const QString &provider);
    /// @brief OCR 提供方变更信号。
    void ocrProviderChanged(const QString &provider);

public slots:
    /// @brief 同步翻译提供方。
    void setTranslationProvider(const QString &provider);
    /// @brief 同步 OCR 提供方。
    void setOcrProvider(const QString &provider);

private:
    /// @brief 刷新各能力实际生效的 provider 状态展示。
    /// @param config 设置配置。
    void refreshProviderStatus(const SettingsConfig &config);

    /// @brief 根据翻译提供方激活对应凭据 Tab。
    /// @param providerId 翻译服务商标识。
    void syncTabToProvider(const QString &providerId);

    QLabel *m_ocrProviderStatus = nullptr;
    QLabel *m_translationProviderStatus = nullptr;
    QLabel *m_codeScanProviderStatus = nullptr;

    QLineEdit *m_codeScanCommand = nullptr;
    QSpinBox *m_codeScanTimeoutMs = nullptr;

    QLineEdit *m_uploadCommand = nullptr;
    QSpinBox *m_uploadTimeoutMs = nullptr;
    QPlainTextEdit *m_uploadEnv = nullptr;

    // OCR 与翻译集成
    QComboBox *m_ocrProvider = nullptr;
    QCheckBox *m_ocrResultPanel = nullptr;
    QComboBox *m_translationProvider = nullptr;
    QTabWidget *m_translationTabs = nullptr;

    // OpenAI 兼容凭据与参数
    QLineEdit *m_translationApiBase = nullptr;
    QLineEdit *m_translationApiKeyEnv = nullptr;
    QLineEdit *m_translationApiKey = nullptr;
    QLineEdit *m_translationModel = nullptr;
    QDoubleSpinBox *m_translationTemperature = nullptr;
    QPlainTextEdit *m_translationSystemPrompt = nullptr;

    // Google Gemini, Anthropic, Tencent, Baidu, Youdao
    CloudTranslateCardWidgets m_cloudTranslate;

    bool m_updatingProviderFromSignal = false;
};

}  // namespace markshot::settings
