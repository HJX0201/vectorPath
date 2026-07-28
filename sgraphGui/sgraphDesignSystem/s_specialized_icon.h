#pragma once

#include "s_icon_provider.h"

class QColor;
class QPainter;

namespace smartGraphics
{

bool drawSpecializedIcon(QPainter& painter, SIconType icon_type, const QColor& foreground,
                         const QColor& accent);

} // namespace smartGraphics
