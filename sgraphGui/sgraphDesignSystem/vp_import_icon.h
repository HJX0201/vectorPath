#pragma once

#include "vp_icon_provider.h"

class QPainter;

namespace Vp
{

bool drawImportIcon(QPainter& painter, VpIconType icon_type, const QColor& foreground,
                    const QColor& accent);

} // namespace Vp
