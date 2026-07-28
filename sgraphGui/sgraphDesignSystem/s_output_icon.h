#pragma once

#include <QColor>

class QPainter;

namespace smartGraphics
{

enum class SIconType;

void drawOutputIcon(QPainter& painter, SIconType icon_type, const QColor& accent_color);

} // namespace smartGraphics
