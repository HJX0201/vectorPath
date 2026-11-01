#pragma once

#include <QColor>

class QPainter;

namespace Vp
{

enum class VpIconType;

// 绘制输出类图标。
void drawOutputIcon(QPainter& painter, VpIconType icon_type, const QColor& accent_color);

} // namespace Vp
