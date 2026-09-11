#include "ocr_result_window/ocr_source_preview.h"

#include "ui/i18n.h"
#include "ui/theme.h"

#include <QBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QResizeEvent>
#include <QToolButton>

namespace markshot::shot {

OcrSourcePreview::OcrSourcePreview(QImage image, QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("ocrSourcePreview"));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    // 1. 【OCR】【原图预览】只保留适合预览的缩略图，避免长期占用完整截图内存
    const QSize originalSize = image.size();
    if (!image.isNull()) {
        image.setDevicePixelRatio(1.0);
        m_image = image.scaled(image.size().boundedTo(QSize(1440, 360)),
                               Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    m_toggle = new QToolButton(this);
    m_toggle->setObjectName(QStringLiteral("ocrSourceToggle"));
    m_toggle->setFont(markshot::theme::uiFont(9));
    m_toggle->setText(MS_TR("Source image") + QStringLiteral("  ·  %1 × %2")
        .arg(originalSize.width()).arg(originalSize.height()));
    m_toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_toggle->setCheckable(true);
    m_toggle->setArrowType(Qt::RightArrow);
    m_toggle->setAccessibleName(MS_TR("Source image"));
    layout->addWidget(m_toggle);

    m_preview = new QLabel(this);
    m_preview->setObjectName(QStringLiteral("ocrSourceThumbnail"));
    m_preview->setAccessibleName(MS_TR("Source image"));
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setFixedHeight(104);
    m_preview->setMinimumWidth(0);
    m_preview->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    layout->addWidget(m_preview);
    connect(m_toggle, &QToolButton::toggled, this, &OcrSourcePreview::setExpanded);
    setExpanded(false);
    setVisible(!image.isNull());
}

void OcrSourcePreview::resizeEvent(QResizeEvent *event)
{
    QFrame::resizeEvent(event);
    updatePreview();
}

void OcrSourcePreview::setExpanded(bool expanded)
{
    m_toggle->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
    m_toggle->setToolTip(expanded ? MS_TR("Hide source image") : MS_TR("Show source image"));
    m_preview->setVisible(expanded);
    if (expanded) {
        updatePreview();
    }
}

void OcrSourcePreview::updatePreview()
{
    if (m_image.isNull() || !m_preview->isVisible()) {
        return;
    }
    const qreal dpr = devicePixelRatioF();
    const QSize target = (m_preview->contentsRect().size() - QSize(12, 12)) * dpr;
    if (target.isEmpty()) {
        return;
    }
    QPixmap pixmap = QPixmap::fromImage(m_image.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    pixmap.setDevicePixelRatio(dpr);
    m_preview->setPixmap(pixmap);
}

}
