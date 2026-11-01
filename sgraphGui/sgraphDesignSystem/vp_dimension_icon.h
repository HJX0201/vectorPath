#pragma once

#include "vp_icon_provider.h"

class QPainter;

namespace Vp
{

// 绘制尺寸标注类图标。
void drawDimensionIcon(QPainter& painter, VpIconType icon_type, const QColor& accent_color);

} // namespace Vp
