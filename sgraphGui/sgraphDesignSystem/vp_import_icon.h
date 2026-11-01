#pragma once

#include "vp_icon_provider.h"

class QPainter;

namespace Vp
{

// 绘制导入类图标。
bool drawImportIcon(QPainter& painter, VpIconType icon_type, const QColor& foreground,
                    const QColor& accent);

} // namespace Vp
