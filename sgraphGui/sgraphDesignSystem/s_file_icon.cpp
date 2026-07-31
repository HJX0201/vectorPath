#include "s_file_icon.h"

#include <QPainter>
#include <QPainterPath>

namespace smartCam
{

void drawFileIcon(QPainter& painter, bool has_plus)
{
    QPainterPath path;
    path.moveTo(16, 7);
    path.lineTo(39, 7);
    path.lineTo(51, 19);
    path.lineTo(51, 57);
    path.lineTo(16, 57);
    path.closeSubpath();
    painter.drawPath(path);
    painter.drawLine(QPointF(39, 7), QPointF(39, 20));
    painter.drawLine(QPointF(39, 20), QPointF(51, 20));
    if (has_plus)
    {
        painter.drawLine(QPointF(25, 37), QPointF(42, 37));
        painter.drawLine(QPointF(33.5, 28.5), QPointF(33.5, 45.5));
    }
}

void drawArrowHead(QPainter& painter, const QPointF& point, bool points_right)
{
    const double direction = points_right ? -1.0 : 1.0;
    painter.drawLine(point, point + QPointF(direction * 9.0, -7.0));
    painter.drawLine(point, point + QPointF(direction * 9.0, 7.0));
}

} // namespace smartCam
