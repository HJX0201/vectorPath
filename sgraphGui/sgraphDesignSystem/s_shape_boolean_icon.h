#pragma once

#include "s_icon_provider.h"

class QColor;
class QPainter;

namespace vectorPath
{

void drawShapeBooleanIcon(QPainter& painter, SIconType icon_type,
                          const QColor& foreground, const QColor& accent_color);

} // namespace vectorPath
