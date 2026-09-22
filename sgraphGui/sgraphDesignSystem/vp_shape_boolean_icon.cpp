#include "vp_shape_boolean_icon.h"

#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <cmath>

namespace Vp
{
namespace
{

constexpr double kPi = 3.14159265358979323846;

void drawRegularPolygon(QPainter& painter, int vertex_count, double inner_ratio = 1.0)
{
    QPolygonF polygon;
    for (int index = 0; index < vertex_count; ++index)
    {
        const double radius = index % 2 == 0 ? 24.0 : 24.0 * inner_ratio;
        const double angle = -kPi / 2.0 + 2.0 * kPi * index / vertex_count;
        polygon << QPointF(32.0 + std::cos(angle) * radius, 32.0 + std::sin(angle) * radius);
    }
    painter.drawPolygon(polygon);
}

void drawBooleanOperands(QPainter& painter, VpIconType icon_type, const QColor& foreground,
                         const QColor& accent_color)
{
    const QRectF left_circle(8, 16, 32, 32);
    const QRectF right_circle(24, 16, 32, 32);
    painter.setPen(QPen(foreground, 3.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(left_circle);
    painter.drawEllipse(right_circle);

    QPainterPath left_path;
    left_path.addEllipse(left_circle);
    QPainterPath right_path;
    right_path.addEllipse(right_circle);
    QPainterPath result_path;
    if (icon_type == VpIconType::BooleanUnion)
    {
        result_path = left_path.united(right_path);
    }
    else if (icon_type == VpIconType::BooleanIntersection)
    {
        result_path = left_path.intersected(right_path);
    }
    else if (icon_type == VpIconType::BooleanDifference)
    {
        result_path = left_path.subtracted(right_path);
    }
    else if (icon_type == VpIconType::BooleanComplement)
    {
        result_path = right_path.subtracted(left_path);
    }
    else
    {
        result_path = left_path.subtracted(right_path).united(right_path.subtracted(left_path));
    }
    painter.setPen(Qt::NoPen);
    QColor fill_color = accent_color;
    fill_color.setAlpha(190);
    painter.setBrush(fill_color);
    painter.drawPath(result_path);
}

} // namespace

void drawShapeBooleanIcon(QPainter& painter, VpIconType icon_type, const QColor& foreground,
                          const QColor& accent_color)
{
    if (icon_type == VpIconType::ShapeStar)
    {
        drawRegularPolygon(painter, 10, 0.3819660112501051);
    }
    else if (icon_type == VpIconType::ShapeTriangle)
    {
        drawRegularPolygon(painter, 3);
    }
    else if (icon_type == VpIconType::ShapePentagon)
    {
        drawRegularPolygon(painter, 5);
    }
    else if (icon_type == VpIconType::ShapeHexagon)
    {
        drawRegularPolygon(painter, 6);
    }
    else if (icon_type == VpIconType::ShapeOctagon)
    {
        drawRegularPolygon(painter, 8);
    }
    else if (icon_type == VpIconType::ShapeDiamond)
    {
        drawRegularPolygon(painter, 4);
    }
    else
    {
        drawBooleanOperands(painter, icon_type, foreground, accent_color);
    }
}

} // namespace Vp
