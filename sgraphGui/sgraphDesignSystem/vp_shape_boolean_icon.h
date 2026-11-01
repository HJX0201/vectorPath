#pragma once

#include "vp_icon_provider.h"

class QColor;
class QPainter;

namespace Vp
{

// 绘制布尔运算类图标。
void drawShapeBooleanIcon(QPainter& painter, VpIconType icon_type, const QColor& foreground,
                          const QColor& accent_color);

} // namespace Vp
