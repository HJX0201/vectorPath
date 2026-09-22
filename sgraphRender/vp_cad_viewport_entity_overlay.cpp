#include "vp_cad_document.h"
#include "vp_cad_viewport.h"
#include "vp_spline_geometry.h"
#include "vp_toolpath.h"

#include <QPainter>
#include <QPolygonF>
#include <cmath>

namespace Vp
{
namespace
{

constexpr double kPi = 3.14159265358979323846;

VpPoint2d polarPoint(const VpPoint2d& center, double radius, double angle_degrees)
{
    const double radians = angle_degrees * kPi / 180.0;
    return {center.x + radius * std::cos(radians), center.y + radius * std::sin(radians)};
}

std::vector<VpPoint2d> entityKeyPoints(const VpEntityRecord& entity)
{
    if (entity.type == VpEntityType::Line)
    {
        const auto& line = std::get<VpLineEntity>(entity.geometry);
        return {line.start_point, line.end_point};
    }
    if (entity.type == VpEntityType::Circle)
    {
        const auto& circle = std::get<VpCircleEntity>(entity.geometry);
        return {circle.center,
                {circle.center.x + circle.radius, circle.center.y},
                {circle.center.x, circle.center.y + circle.radius},
                {circle.center.x - circle.radius, circle.center.y},
                {circle.center.x, circle.center.y - circle.radius}};
    }
    if (entity.type == VpEntityType::Arc)
    {
        const auto& arc = std::get<VpArcEntity>(entity.geometry);
        return {arc.center, polarPoint(arc.center, arc.radius, arc.start_angle),
                polarPoint(arc.center, arc.radius, arc.end_angle)};
    }
    if (entity.type == VpEntityType::Polyline)
    {
        return std::get<VpPolylineEntity>(entity.geometry).vertices;
    }
    if (entity.type == VpEntityType::Spline)
    {
        const auto& points = std::get<VpSplineEntity>(entity.geometry).control_points;
        return std::vector<VpPoint2d>(points.begin(), points.end());
    }
    if (entity.type == VpEntityType::Ellipse)
    {
        const auto& ellipse = std::get<VpEllipseEntity>(entity.geometry);
        return {ellipse.center,
                {ellipse.center.x + ellipse.major_axis.x, ellipse.center.y + ellipse.major_axis.y},
                {ellipse.center.x - ellipse.major_axis.x, ellipse.center.y - ellipse.major_axis.y},
                {ellipse.center.x + ellipse.minor_axis.x, ellipse.center.y + ellipse.minor_axis.y},
                {ellipse.center.x - ellipse.minor_axis.x, ellipse.center.y - ellipse.minor_axis.y}};
    }
    if (entity.type == VpEntityType::Text)
    {
        return {std::get<VpTextEntity>(entity.geometry).position};
    }
    if (entity.type == VpEntityType::MText)
    {
        return {std::get<VpMTextEntity>(entity.geometry).position};
    }
    if (entity.type == VpEntityType::LinearDimension)
    {
        const auto& dimension = std::get<VpLinearDimensionEntity>(entity.geometry);
        return {dimension.first_point, dimension.second_point, dimension.dimension_line_point,
                dimension.center_point};
    }
    if (entity.type == VpEntityType::Hatch)
    {
        return std::get<VpHatchEntity>(entity.geometry).boundary;
    }
    if (entity.type == VpEntityType::Leader)
    {
        return std::get<VpLeaderEntity>(entity.geometry).vertices;
    }
    return {};
}

bool directionVector(const VpEntityRecord& entity, VpPoint2d& point, VpPoint2d& vector)
{
    if (entity.type == VpEntityType::Line)
    {
        const auto& line = std::get<VpLineEntity>(entity.geometry);
        point = {(line.start_point.x + line.end_point.x) * 0.5,
                 (line.start_point.y + line.end_point.y) * 0.5};
        vector = {line.end_point.x - line.start_point.x, line.end_point.y - line.start_point.y};
        return true;
    }
    if (entity.type == VpEntityType::Arc)
    {
        const auto& arc = std::get<VpArcEntity>(entity.geometry);
        double span = std::fmod(arc.is_clockwise ? arc.start_angle - arc.end_angle
                                                 : arc.end_angle - arc.start_angle,
                                360.0);
        if (span < 0.0)
        {
            span += 360.0;
        }
        const double angle = arc.start_angle + (arc.is_clockwise ? -0.5 : 0.5) * span;
        point = polarPoint(arc.center, arc.radius, angle);
        const double radians = angle * kPi / 180.0;
        vector = arc.is_clockwise ? VpPoint2d{std::sin(radians), -std::cos(radians)}
                                  : VpPoint2d{-std::sin(radians), std::cos(radians)};
        return true;
    }
    if (entity.type == VpEntityType::Polyline)
    {
        const auto& vertices = std::get<VpPolylineEntity>(entity.geometry).vertices;
        for (std::size_t index = 1; index < vertices.size(); ++index)
        {
            vector = {vertices[index].x - vertices[index - 1].x,
                      vertices[index].y - vertices[index - 1].y};
            if (std::hypot(vector.x, vector.y) > 1.0e-9)
            {
                point = {(vertices[index].x + vertices[index - 1].x) * 0.5,
                         (vertices[index].y + vertices[index - 1].y) * 0.5};
                return true;
            }
        }
        return false;
    }
    if (entity.type == VpEntityType::Spline)
    {
        const auto& spline = std::get<VpSplineEntity>(entity.geometry);
        point = splinePoint(spline, 0.5);
        vector = splineTangent(spline, 0.5);
        return true;
    }
    if (entity.type == VpEntityType::Circle)
    {
        const auto& circle = std::get<VpCircleEntity>(entity.geometry);
        point = {circle.center.x + circle.radius, circle.center.y};
        vector = {0.0, 1.0};
        return true;
    }
    if (entity.type == VpEntityType::Ellipse)
    {
        const auto& ellipse = std::get<VpEllipseEntity>(entity.geometry);
        point = {ellipse.center.x + ellipse.major_axis.x, ellipse.center.y + ellipse.major_axis.y};
        vector = ellipse.minor_axis;
        return true;
    }
    return false;
}

} // namespace

void VpCadViewport::drawEntityDisplayOverlay(QPainter& painter)
{
    if (!m_document || (!m_is_node_display_visible && !m_is_direction_display_visible &&
                        !m_is_sequence_display_visible))
    {
        return;
    }
    painter.save();
    for (std::size_t index = 0; index < m_document->entities().size(); ++index)
    {
        const VpEntityRecord& entity = m_document->entities()[index];
        if (!m_document->isLayerVisible(entity.layer_name))
        {
            continue;
        }
        const std::vector<VpPoint2d> key_points = entityKeyPoints(entity);
        if (m_is_node_display_visible)
        {
            painter.setPen(QPen(QColor(74, 216, 255), 1.2));
            painter.setBrush(QColor(8, 11, 15, 210));
            for (const VpPoint2d& key_point : key_points)
            {
                painter.drawRect(
                    QRectF(worldToScreen(key_point) - QPointF(3.0, 3.0), QSizeF(6.0, 6.0)));
            }
        }
        if (m_is_direction_display_visible && isMachinableEntity(entity))
        {
            VpPoint2d arrow_point;
            VpPoint2d arrow_vector;
            if (directionVector(entity, arrow_point, arrow_vector))
            {
                const QPointF center = worldToScreen(arrow_point);
                QPointF direction(arrow_vector.x, -arrow_vector.y);
                const double length = std::hypot(direction.x(), direction.y());
                if (length > 1.0e-9)
                {
                    direction /= length;
                    const QPointF normal(-direction.y(), direction.x());
                    const QPointF tip = center + direction * 8.0;
                    QPolygonF arrow;
                    arrow << tip << center - direction * 5.0 + normal * 5.0
                          << center - direction * 5.0 - normal * 5.0;
                    painter.setPen(QPen(QColor(255, 184, 72), 1.0));
                    painter.setBrush(QColor(255, 184, 72, 220));
                    painter.drawPolygon(arrow);
                }
            }
        }
        if (m_is_sequence_display_visible)
        {
            const VpPoint2d anchor = isMachinableEntity(entity)
                                         ? toolpathStartPoint(entity)
                                         : (key_points.empty() ? VpPoint2d{} : key_points.front());
            const QPointF top_left = worldToScreen(anchor) + QPointF(7.0, -21.0);
            const QRectF label_rect(top_left, QSizeF(38.0, 18.0));
            painter.setPen(QPen(m_overlay_text_color, 1.0));
            painter.setBrush(
                QColor(m_canvas_color.red(), m_canvas_color.green(), m_canvas_color.blue(), 220));
            painter.drawRoundedRect(label_rect, 2.0, 2.0);
            painter.drawText(label_rect, Qt::AlignCenter,
                             QString::number(static_cast<qulonglong>(index + 1)));
        }
    }
    painter.restore();
}

} // namespace Vp
