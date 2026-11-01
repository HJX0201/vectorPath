#pragma once

#include "vp_icon_provider.h"

class QColor;
class QPainter;

namespace Vp
{

// 绘制快速变换类图标。
void drawQuickTransformIcon(QPainter& painter, VpIconType icon_type, const QColor& foreground,
                            const QColor& accent);

} // namespace Vp
