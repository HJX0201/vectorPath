#include "s_dimension_icon.h"

#include <QPainter>
#include <QPolygonF>

namespace smartGraphics
{
namespace
{

void drawArrow(QPainter& painter, const QPointF& tip, const QPointF& direction)
{
    QPolygonF arrow;
    arrow << tip << tip + direction + QPointF(-direction.y() * 0.45, direction.x() * 0.45)
          << tip + direction + QPointF(direction.y() * 0.45, -direction.x() * 0.45);
    painter.drawPolygon(arrow);
}

} // namespace

void drawDimensionIcon(QPainter& painter, SIconType icon_type, const QColor& accent_color)
{
    painter.save();
    painter.setBrush(accent_color);
    if (icon_type == SIconType::Dimension || icon_type == SIconType::DimensionAligned)
    {
        const bool aligned = icon_type == SIconType::DimensionAligned;
        const QPointF first = aligned ? QPointF(14, 45) : QPointF(14, 32);
        const QPointF second = aligned ? QPointF(50, 19) : QPointF(50, 32);
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(aligned ? QPointF(10, 24) : QPointF(12, 14),
                         aligned ? QPointF(24, 54) : QPointF(12, 51));
        painter.drawLine(aligned ? QPointF(40, 10) : QPointF(52, 14),
                         aligned ? QPointF(55, 41) : QPointF(52, 51));
        painter.drawLine(first, second);
        painter.setBrush(accent_color);
        drawArrow(painter, first, (second - first) / 5.0);
        drawArrow(painter, second, (first - second) / 5.0);
    }
    else if (icon_type == SIconType::DimensionAngular || icon_type == SIconType::DimensionArcLength)
    {
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(QPointF(13, 50), QPointF(32, 31));
        painter.drawLine(QPointF(32, 31), QPointF(54, 49));
        painter.drawArc(QRectF(19, 28, 27, 27), 35 * 16, 110 * 16);
        if (icon_type == SIconType::DimensionArcLength)
        {
            painter.setPen(QPen(accent_color, 3.0));
            painter.drawArc(QRectF(13, 8, 38, 20), 15 * 16, 150 * 16);
        }
    }
    else if (icon_type == SIconType::DimensionRadius || icon_type == SIconType::DimensionDiameter)
    {
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(QRectF(11, 11, 42, 42));
        painter.drawLine(icon_type == SIconType::DimensionDiameter ? QPointF(16, 48)
                                                                   : QPointF(32, 32),
                         QPointF(48, 16));
        painter.setBrush(accent_color);
        drawArrow(painter, QPointF(48, 16), QPointF(-7, 7));
        painter.drawText(QPointF(5, 61), icon_type == SIconType::DimensionDiameter
                                             ? QString(QChar(0x2300))
                                             : QStringLiteral("R"));
    }
    else if (icon_type == SIconType::DimensionOrdinate)
    {
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(QPointF(10, 53), QPointF(10, 14));
        painter.drawLine(QPointF(10, 53), QPointF(53, 53));
        painter.drawLine(QPointF(25, 37), QPointF(45, 37));
        painter.drawLine(QPointF(45, 37), QPointF(45, 20));
        painter.setBrush(accent_color);
        painter.drawEllipse(QPointF(25, 37), 4, 4);
    }
    else if (icon_type == SIconType::DimensionStyle)
    {
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(QPointF(10, 17), QPointF(54, 17));
        painter.drawLine(QPointF(10, 42), QPointF(54, 42));
        painter.setPen(QPen(accent_color, 3.0));
        painter.drawEllipse(QPointF(43, 48), 9, 9);
        painter.drawLine(QPointF(43, 34), QPointF(43, 39));
        painter.drawLine(QPointF(43, 57), QPointF(43, 62));
        painter.drawLine(QPointF(29, 48), QPointF(34, 48));
        painter.drawLine(QPointF(52, 48), QPointF(57, 48));
    }
    painter.restore();
}

} // namespace smartGraphics
