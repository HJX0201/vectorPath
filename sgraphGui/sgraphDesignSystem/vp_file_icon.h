#pragma once

class QPainter;
class QPointF;

namespace Vp
{

void drawFileIcon(QPainter& painter, bool has_plus);
void drawArrowHead(QPainter& painter, const QPointF& point, bool points_right);

} // namespace Vp
