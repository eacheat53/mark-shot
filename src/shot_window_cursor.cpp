#include "shot_window_module.h"

using namespace markshot::shot;

namespace {

/// @brief 将选区或标注操作映射为缩放、抓取或控制点光标
/// @param drag 当前命中的操作类型
/// @param pressed 是否正在按住指针执行操作
/// @return 与操作方向和拖动状态一致的系统光标
Qt::CursorShape cursorForSelectionDrag(types::SelectionDrag drag, bool pressed)
{
    using Drag = types::SelectionDrag;
    switch (drag) {
    case Drag::Left:
    case Drag::Right:
    case Drag::MagnifierSourceLeft:
    case Drag::MagnifierSourceRight:
        return Qt::SizeHorCursor;
    case Drag::Top:
    case Drag::Bottom:
    case Drag::MagnifierSourceTop:
    case Drag::MagnifierSourceBottom:
        return Qt::SizeVerCursor;
    case Drag::TopLeft:
    case Drag::BottomRight:
    case Drag::MagnifierSourceTopLeft:
    case Drag::MagnifierSourceBottomRight:
        return Qt::SizeFDiagCursor;
    case Drag::TopRight:
    case Drag::BottomLeft:
    case Drag::MagnifierSourceTopRight:
    case Drag::MagnifierSourceBottomLeft:
        return Qt::SizeBDiagCursor;
    case Drag::LineControl:
    case Drag::LineStart:
    case Drag::LineEnd:
    case Drag::NumberTip:
        return Qt::SizeAllCursor;
    case Drag::Move:
    case Drag::Rotate:
    case Drag::MagnifierSource:
    case Drag::MagnifierLens:
    case Drag::NumberBubble:
        return pressed ? Qt::ClosedHandCursor : Qt::OpenHandCursor;
    case Drag::None:
        return Qt::ArrowCursor;
    }
    return Qt::ArrowCursor;
}

}

void ShotWindow::updateCursor()
{
    updatePointerCursor(mapFromGlobal(QCursor::pos()));
}

void ShotWindow::updatePointerCursor(QPointF widgetPoint)
{
    if (m_operationBusy) {
        setCursor(Qt::BusyCursor);
        return;
    }
    // 1. 【截图】【光标状态】进行中的抓取保持闭合手型，界面控件优先使用各自光标
    if (m_toolbarDragging || m_imagePanning) {
        setCursor(Qt::ClosedHandCursor);
        return;
    }
    if (m_activeRecordingStopHovered && !m_dragging) {
        setCursor(Qt::PointingHandCursor);
        return;
    }
    if (!m_dragging && (propertyComboPopupVisible() || mouseOverUiWidget())) {
        setCursor(Qt::ArrowCursor);
        return;
    }

    // 2. 【截图】【精确定位】软件指针或画笔预览接管显示时才隐藏系统光标
    if (((m_mode == Mode::Selecting || canAdjustSelection()) && m_selectionPointerDetached)
        || (m_showWheelPreview && m_wheelPreviewTimer.isValid() && m_wheelPreviewTimer.elapsed() <= 900)) {
        setCursor(Qt::BlankCursor);
        return;
    }
    if (m_mode == Mode::Selecting) {
        setCursor(captureCrossCursor());
        return;
    }
    if (!m_dragging && !m_frozenImageRect.contains(widgetPoint)) {
        setCursor(Qt::ArrowCursor);
        return;
    }

    // 3. 【截图】【选区操作】空闲时重新命中当前位置，结束拖动后立即恢复悬停语义
    const QPointF imagePoint = widgetToImage(widgetPoint);
    if (m_tool == Tool::Move && !m_fullscreenAnnotation) {
        const SelectionDrag drag = m_dragging ? m_selectionDrag : selectionDragAt(imagePoint);
        setCursor(cursorForSelectionDrag(drag, m_dragging));
        return;
    }
    if (m_tool == Tool::Select) {
        if (!m_dragging) {
            m_annotationDrag = SelectionDrag::None;
            if (selectedAnnotationIds().size() > 1) {
                m_annotationDrag = selectedAnnotationsDragAt(imagePoint);
            } else if (m_selectedAnnotationId.has_value()) {
                m_annotationDrag = annotationDragAt(imagePoint, *m_selectedAnnotationId);
            }
            if (m_annotationDrag == SelectionDrag::None && annotationAt(imagePoint).has_value()) {
                m_annotationDrag = SelectionDrag::Move;
            }
        }
        if (m_annotationDrag != SelectionDrag::None) {
            setCursor(cursorForSelectionDrag(m_annotationDrag, m_dragging));
        } else if (m_imageNavigationEnabled && m_imageSelected) {
            setCursor(Qt::OpenHandCursor);
        } else {
            setCursor(m_annotationSelectionBoxActive ? Qt::CrossCursor : Qt::ArrowCursor);
        }
        return;
    }

    // 4. 【截图】【标注工具】文字使用插入光标，其余绘制工具使用精确十字
    setCursor(m_tool == Tool::Text ? Qt::IBeamCursor : captureCrossCursor());
}
