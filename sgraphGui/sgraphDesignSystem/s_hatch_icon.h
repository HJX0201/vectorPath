#pragma once

#include "s_icon_provider.h"

class QPainter;

namespace smartCam
{

void drawHatchIcon(QPainter& painter, SIconType icon_type, const QColor& accent_color);

} // namespace smartCam
