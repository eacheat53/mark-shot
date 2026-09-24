#pragma once

class QFormLayout;
class QVBoxLayout;

namespace markshot::settings {

/// @brief 为设置分组创建统一字段起点、长标签换行和窄窗口上下排列的表单
/// @param parentLayout 接收表单的分组布局
/// @return 已添加到分组内的表单布局
QFormLayout *createSettingsFormLayout(QVBoxLayout *parentLayout);

}
