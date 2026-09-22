#pragma once

#include "vp_icon_provider.h"

class QColor;
class QPainter;

namespace Vp
{

void drawShapeBooleanIcon(QPainter& painter, VpIconType icon_type, const QColor& foreground,
                          const QColor& accent_color);

} // namespace Vp
