#pragma once

#include "vp_icon_provider.h"

class QPainter;

namespace Vp
{

void drawDimensionIcon(QPainter& painter, VpIconType icon_type, const QColor& accent_color);

} // namespace Vp
