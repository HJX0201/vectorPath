#pragma once

#include "s_icon_provider.h"

class QPainter;

namespace smartGraphics
{

void drawDimensionIcon(QPainter& painter, SIconType icon_type, const QColor& accent_color);

} // namespace smartGraphics
