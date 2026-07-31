#pragma once

#include "s_icon_provider.h"

class QColor;
class QPainter;

namespace smartCam
{

bool drawSpecializedIcon(QPainter& painter, SIconType icon_type, const QColor& foreground,
                         const QColor& accent);

} // namespace smartCam
