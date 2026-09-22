#pragma once

#include <QColor>

class QPainter;

namespace Vp
{

enum class VpIconType;

void drawOutputIcon(QPainter& painter, VpIconType icon_type, const QColor& accent_color);

} // namespace Vp
