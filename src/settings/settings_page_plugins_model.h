#pragma once

#include "providers/provider_plugin_info.h"

#include <QString>
#include <QStringList>
#include <QVector>

class QComboBox;

namespace markshot::settings {

struct ProviderOption {
    QString label;
    QString value;
};

struct PluginDiagnosticRow {
    QString capability;
    QString provider;
    QString status;
    QString path;
    QString details;
};

/**
 * 生成指定能力的 provider 选择项。
 * @param capability 插件能力。
 * @return 下拉框选择项。
 */
QVector<ProviderOption> providerOptionsForCapability(markshot::providers::ProviderPluginCapability capability);

/**
 * 生成插件诊断表格行。
 * @return 诊断表格行。
 */
QVector<PluginDiagnosticRow> pluginDiagnosticRows();

/**
 * 读取插件搜索目录列表。
 * @return 插件搜索目录。
 */
QStringList pluginSearchDirectoryRows();

/**
 * 读取用户级插件目录。
 * @return 用户级插件目录。
 */
QString userPluginDirectory();

/**
 * 填充 provider 下拉框。
 * @param combo 下拉框控件。
 * @param capability 插件能力。
 */
void populateProviderCombo(QComboBox *combo, markshot::providers::ProviderPluginCapability capability);

/**
 * 设置 provider 下拉框当前值。
 * @param combo 下拉框控件。
 * @param value provider 配置值。
 */
void setProviderComboValue(QComboBox *combo, const QString &value);

/**
 * 读取 provider 下拉框当前值。
 * @param combo 下拉框控件。
 * @return provider 配置值。
 */
QString providerComboValue(const QComboBox *combo);

}  // namespace markshot::settings
