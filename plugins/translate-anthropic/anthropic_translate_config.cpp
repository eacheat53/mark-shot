#include "anthropic_translate_config.h"

#include "translate_config_source.h"

#include <QStringList>

namespace markshot::translate_anthropic {
namespace {

using markshot::translate_common::TranslateConfigSource;

/**
 * 读取 API Key 的环境变量候选名。
 * @return 按优先级排列的环境变量名列表。
 */
QStringList apiKeyEnvNames()
{
    return {QStringLiteral("ANTHROPIC_API_KEY"), QStringLiteral("MARK_SHOT_ANTHROPIC_API_KEY")};
}

/**
 * 读取 Model 的环境变量候选名。
 * @return 环境变量名列表。
 */
QStringList modelEnvNames()
{
    return {QStringLiteral("ANTHROPIC_MODEL"), QStringLiteral("MARK_SHOT_ANTHROPIC_MODEL")};
}

/**
 * 读取 Endpoint 的环境变量候选名。
 * @return 环境变量名列表。
 */
QStringList endpointEnvNames()
{
    return {QStringLiteral("ANTHROPIC_API_BASE"), QStringLiteral("MARK_SHOT_ANTHROPIC_API_BASE")};
}

double configDouble(const TranslateConfigSource &source, const QString &key, double fallback)
{
    double value = fallback;
    if (source.translation.contains(key)) {
        value = source.translation.value(key).toDouble(value);
    }
    if (source.vendor.contains(key)) {
        value = source.vendor.value(key).toDouble(value);
    }
    return value;
}

}  // namespace

QString defaultSystemPrompt()
{
    return QStringLiteral(
        "You are a professional native translator. Fluently translate OCR text segments into the target language. "
        "Preserve original meaning, keep segment count and ids unchanged. "
        "Keep proper nouns, brand names, code snippets, formulas, and numbers intact. "
        "Return only valid JSON without extra text or explanations.");
}

AnthropicTranslateConfig readAnthropicTranslateConfig()
{
    const TranslateConfigSource source =
        markshot::translate_common::readTranslateConfigSource(QStringLiteral("anthropic"));

    AnthropicTranslateConfig result;

    result.apiKey = markshot::translate_common::configString(source, QStringLiteral("apiKey"), apiKeyEnvNames());
    result.model = markshot::translate_common::configString(source, QStringLiteral("model"), modelEnvNames(), result.model);
    result.endpoint = markshot::translate_common::configString(source, QStringLiteral("endpoint"), endpointEnvNames(), result.endpoint);
    result.systemPrompt = markshot::translate_common::configString(source,
                                                                   QStringLiteral("systemPrompt"),
                                                                   {},
                                                                   defaultSystemPrompt());
    result.temperature = configDouble(source, QStringLiteral("temperature"), result.temperature);
    result.maxTokens = markshot::translate_common::configInt(source,
                                                             QStringLiteral("maxTokens"),
                                                             result.maxTokens,
                                                             256,
                                                             8192);
    result.timeoutMs = markshot::translate_common::configInt(source,
                                                             QStringLiteral("timeoutMs"),
                                                             result.timeoutMs,
                                                             1000,
                                                             300000);
    return result;
}

bool validateAnthropicTranslateConfig(const AnthropicTranslateConfig &config, QString *error)
{
    if (config.endpoint.trimmed().isEmpty()) {
        if (error) {
            *error = QStringLiteral("missing anthropic endpoint");
        }
        return false;
    }
    if (config.model.trimmed().isEmpty()) {
        if (error) {
            *error = QStringLiteral("missing anthropic model");
        }
        return false;
    }
    if (config.apiKey.trimmed().isEmpty()) {
        if (error) {
            *error = QStringLiteral("missing anthropic apiKey: set translation.anthropic.apiKey or ANTHROPIC_API_KEY");
        }
        return false;
    }
    return true;
}

}  // namespace markshot::translate_anthropic
