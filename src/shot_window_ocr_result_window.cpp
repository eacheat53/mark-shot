#include "shot_window_module.h"

#include "ocr_result_window/ocr_result_window.h"

#include <utility>

namespace markshot::shot {

QWidget *createOcrResultWindow(QString text, QScreen *targetScreen, QImage sourceImage)
{
    return new OcrResultWindow(std::move(text), targetScreen, std::move(sourceImage));
}

}  // namespace markshot::shot
