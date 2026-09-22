#pragma once

#include "vp_icon_provider.h"

class QColor;
class QPainter;

namespace Vp
{

void drawQuickTransformIcon(QPainter& painter, VpIconType icon_type, const QColor& foreground,
                            const QColor& accent);

} // namespace Vp
