#pragma once

class QPainter;
class QPointF;

namespace smartGraphics
{

void drawFileIcon(QPainter& painter, bool has_plus);
void drawArrowHead(QPainter& painter, const QPointF& point, bool points_right);

} // namespace smartGraphics
