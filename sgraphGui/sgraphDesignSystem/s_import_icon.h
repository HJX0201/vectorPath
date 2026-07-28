#pragma once

#include "s_icon_provider.h"

class QPainter;

namespace smartGraphics
{

bool drawImportIcon(QPainter& painter, SIconType icon_type,
                    const QColor& foreground, const QColor& accent);

} // namespace smartGraphics
