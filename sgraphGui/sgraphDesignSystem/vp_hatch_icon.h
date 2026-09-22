#pragma once

#include "vp_icon_provider.h"

class QPainter;

namespace Vp
{

void drawHatchIcon(QPainter& painter, VpIconType icon_type, const QColor& accent_color);

} // namespace Vp
