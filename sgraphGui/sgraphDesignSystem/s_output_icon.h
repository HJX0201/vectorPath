#pragma once

#include <QColor>

class QPainter;

namespace vectorPath
{

enum class SIconType;

void drawOutputIcon(QPainter& painter, SIconType icon_type, const QColor& accent_color);

} // namespace vectorPath
