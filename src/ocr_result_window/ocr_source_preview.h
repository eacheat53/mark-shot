#pragma once

#include <QFrame>
#include <QImage>

class QLabel;
class QToolButton;

namespace markshot::shot {

/// @brief 按原始比例显示截图缩略图，默认折叠以保留文本空间
class OcrSourcePreview final : public QFrame {
public:
    /// @brief 创建原图预览，空图片不占用窗口空间
    /// @param image 本次 OCR 实际使用的截图
    /// @param parent 所属窗口
    explicit OcrSourcePreview(QImage image, QWidget *parent = nullptr);

protected:
    /// @brief 调整窗口尺寸后重新生成与屏幕缩放匹配的缩略图
    /// @param event 尺寸变化事件
    /// @return 无返回值
    void resizeEvent(QResizeEvent *event) override;

public:
    /// @brief 展开或折叠原图预览
    /// @param expanded 是否显示缩略图
    /// @return 无返回值
    void setExpanded(bool expanded);

private:
    /// @brief 将缓存图片缩放到预览区，不拉伸原始比例
    /// @return 无返回值
    void updatePreview();

    QImage m_image;
    QLabel *m_preview = nullptr;
    QToolButton *m_toggle = nullptr;
};

}
