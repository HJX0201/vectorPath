#pragma once

#include <QColor>

class QPainter;

namespace smartCam
{

enum class SIconType;

void drawOutputIcon(QPainter& painter, SIconType icon_type, const QColor& accent_color);

} // namespace smartCam
