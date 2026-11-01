#pragma once

#include "vp_icon_provider.h"

class QColor;
class QPainter;

namespace Vp
{

// 绘制专业工具专用的图标。
bool drawSpecializedIcon(QPainter& painter, VpIconType icon_type, const QColor& foreground,
                         const QColor& accent);

} // namespace Vp
