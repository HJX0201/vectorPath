#pragma once

#include "vp_icon_provider.h"

class QPainter;

namespace Vp
{

// 绘制填充类图标。
void drawHatchIcon(QPainter& painter, VpIconType icon_type, const QColor& accent_color);

} // namespace Vp
