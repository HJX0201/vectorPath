#include "s_quick_transform_icon.h"

#include <QPainter>
#include <QPainterPath>

namespace smartCam
{
namespace
{

void drawArrowHead(QPainter& painter, const QPointF& point, bool clockwise)
{
    QPainterPath arrow;
    arrow.moveTo(point);
    arrow.lineTo(point + QPointF(clockwise ? -10.0 : 10.0, -2.0));
    arrow.lineTo(point + QPointF(clockwise ? -3.0 : 3.0, 8.0));
    arrow.closeSubpath();
    painter.drawPath(arrow);
}

void drawRotation(QPainter& painter, bool clockwise, bool half_turn, const QColor& accent)
{
    painter.drawArc(QRectF(11, 11, 42, 42), clockwise ? 35 * 16 : 55 * 16,
                    (clockwise ? -1 : 1) * (half_turn ? 250 : 205) * 16);
    painter.setPen(Qt::NoPen);
    painter.setBrush(accent);
    drawArrowHead(painter, clockwise ? QPointF(50, 20) : QPointF(14, 20), clockwise);
    painter.setPen(QPen(accent, 3.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawText(QRectF(17, 22, 30, 22), Qt::AlignCenter,
                     half_turn ? QStringLiteral("180") : QStringLiteral("90"));
}

} // namespace

void drawQuickTransformIcon(QPainter& painter, SIconType icon_type, const QColor& foreground,
                            const QColor& accent)
{
    if (icon_type == SIconType::QuickReverse)
    {
        painter.drawLine(QPointF(12, 19), QPointF(51, 19));
        painter.drawLine(QPointF(51, 19), QPointF(43, 11));
        painter.drawLine(QPointF(12, 45), QPointF(51, 45));
        painter.drawLine(QPointF(12, 45), QPointF(20, 37));
        painter.setPen(QPen(accent, 4.0));
        painter.drawLine(QPointF(32, 25), QPointF(32, 39));
        return;
    }
    if (icon_type == SIconType::MirrorVertical || icon_type == SIconType::MirrorHorizontal)
    {
        const bool vertical = icon_type == SIconType::MirrorVertical;
        painter.setPen(QPen(accent, 3.0, Qt::DashLine));
        painter.drawLine(vertical ? QPointF(32, 7) : QPointF(7, 32),
                         vertical ? QPointF(32, 57) : QPointF(57, 32));
        painter.setPen(QPen(foreground, 3.0));
        if (vertical)
        {
            painter.drawPolygon(QPolygonF({QPointF(12, 16), QPointF(27, 23), QPointF(27, 48)}));
            painter.drawPolygon(QPolygonF({QPointF(52, 16), QPointF(37, 23), QPointF(37, 48)}));
        }
        else
        {
            painter.drawPolygon(QPolygonF({QPointF(16, 12), QPointF(23, 27), QPointF(48, 27)}));
            painter.drawPolygon(QPolygonF({QPointF(16, 52), QPointF(23, 37), QPointF(48, 37)}));
        }
        return;
    }
    const bool clockwise = icon_type == SIconType::RotateClockwise90 ||
                           icon_type == SIconType::RotateClockwise180;
    const bool half_turn = icon_type == SIconType::RotateClockwise180 ||
                           icon_type == SIconType::RotateCounterclockwise180;
    drawRotation(painter, clockwise, half_turn, accent);
}

} // namespace smartCam
