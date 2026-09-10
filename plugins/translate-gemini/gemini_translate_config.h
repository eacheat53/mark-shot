#pragma once

#include <QString>

namespace markshot::translate_gemini {

struct GeminiTranslateConfig {
    QString endpoint = QStringLiteral("https://generativelanguage.googleapis.com/v1beta");
    QString apiKey;
    QString model = QStringLiteral("gemini-2.5-flash");
    QString systemPrompt;
    double temperature = 0.2;
    int timeoutMs = 60000;
};

/**
 * 读取 Google Gemini 翻译插件配置。
 * @return 合并应用配置、环境变量与默认值后的配置。
 */
GeminiTranslateConfig readGeminiTranslateConfig();

/**
 * 校验翻译配置是否具备发起请求的必要字段。
 * @param config 待校验配置。
 * @param error 输出错误信息。
 * @return 配置可用时返回 true。
 */
bool validateGeminiTranslateConfig(const GeminiTranslateConfig &config, QString *error);

/**
 * 读取默认翻译系统提示词。
 * @return 系统提示词。
 */
QString defaultSystemPrompt();

}  // namespace markshot::translate_gemini
