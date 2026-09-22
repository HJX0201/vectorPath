#include "vp_cad_document.h"
#include "vp_cad_viewport.h"
#include "vp_cad_viewport_geometry.h"
#include "vp_document_transaction.h"

#include <QPainter>
#include <algorithm>
#include <cmath>
#include <limits>

namespace Vp
{
namespace
{

constexpr double kPi = 3.14159265358979323846;

VpPoint2d arcPoint(const VpArcEntity& arc, double angle)
{
    const double radians = angle * kPi / 180.0;
    return {arc.center.x + (arc.radius * std::cos(radians)),
            arc.center.y + (arc.radius * std::sin(radians))};
}

double normalizedAngle(double angle)
{
    const double result = std::fmod(angle, 360.0);
    return result < 0.0 ? result + 360.0 : result;
}

void appendGrip(std::vector<VpGripHandle>& grips, const VpEntityRecord& entity, VpGripRole role,
                std::size_t index, const VpPoint2d& point)
{
    grips.push_back({entity.id, role, index, point});
}

QString gripOperationName(VpGripOperation operation)
{
    switch (operation)
    {
    case VpGripOperation::Stretch:
        return QStringLiteral("拉伸");
    case VpGripOperation::Move:
        return QStringLiteral("移动");
    case VpGripOperation::Rotate:
        return QStringLiteral("旋转");
    case VpGripOperation::Scale:
        return QStringLiteral("缩放");
    case VpGripOperation::Mirror:
        return QStringLiteral("镜像");
    }
    return {};
}

VpPoint2d selectionGripReference(const std::vector<VpEntityRecord>& sources,
                                 const VpPoint2d& base_point)
{
    VpPoint2d result;
    std::size_t point_count = 0;
    for (const VpEntityRecord& source : sources)
    {
        for (const VpGripHandle& grip : entityGripHandles(source))
        {
            result.x += grip.point.x;
            result.y += grip.point.y;
            ++point_count;
        }
    }
    if (point_count > 0)
    {
        result.x /= static_cast<double>(point_count);
        result.y /= static_cast<double>(point_count);
    }
    if (point_count == 0 || distance(result, base_point) <= 1.0e-9)
    {
        result = {base_point.x + 1.0, base_point.y};
    }
    return result;
}

} // namespace

std::vector<VpEntityRecord> gripTransformedEntities(const std::vector<VpEntityRecord>& sources,
                                                    VpGripOperation operation,
                                                    const VpPoint2d& base_point,
                                                    const VpPoint2d& reference_point,
                                                    const VpPoint2d& destination)
{
    std::vector<VpEntityRecord> results;
    results.reserve(sources.size());
    const double reference_distance = distance(base_point, reference_point);
    const double destination_distance = distance(base_point, destination);
    const double reference_angle = entityAngleDegrees(base_point, reference_point);
    const double destination_angle = entityAngleDegrees(base_point, destination);
    for (const VpEntityRecord& source : sources)
    {
        if (operation == VpGripOperation::Move)
        {
            results.push_back(translatedEntity(source, destination.x - base_point.x,
                                               destination.y - base_point.y));
        }
        else if (operation == VpGripOperation::Rotate && destination_distance > 1.0e-9)
        {
            results.push_back(
                rotatedEntity(source, base_point, destination_angle - reference_angle));
        }
        else if (operation == VpGripOperation::Scale && reference_distance > 1.0e-9 &&
                 destination_distance > 1.0e-9)
        {
            results.push_back(
                scaledEntity(source, base_point, destination_distance / reference_distance));
        }
        else if (operation == VpGripOperation::Mirror && destination_distance > 1.0e-9)
        {
            results.push_back(mirroredEntity(source, base_point, destination));
        }
        else
        {
            results.push_back(source);
        }
    }
    return results;
}

std::vector<VpGripHandle> entityGripHandles(const VpEntityRecord& entity)
{
    std::vector<VpGripHandle> grips;
    if (entity.type == VpEntityType::Line)
    {
        const auto& line = std::get<VpLineEntity>(entity.geometry);
        appendGrip(grips, entity, VpGripRole::ControlPoint, 0, line.start_point);
        appendGrip(grips, entity, VpGripRole::MoveEntity, 0,
                   {(line.start_point.x + line.end_point.x) * 0.5,
                    (line.start_point.y + line.end_point.y) * 0.5});
        appendGrip(grips, entity, VpGripRole::ControlPoint, 1, line.end_point);
    }
    else if (entity.type == VpEntityType::Circle)
    {
        const auto& circle = std::get<VpCircleEntity>(entity.geometry);
        appendGrip(grips, entity, VpGripRole::MoveEntity, 0, circle.center);
        appendGrip(grips, entity, VpGripRole::RadiusPoint, 0,
                   {circle.center.x + circle.radius, circle.center.y});
        appendGrip(grips, entity, VpGripRole::RadiusPoint, 1,
                   {circle.center.x, circle.center.y + circle.radius});
        appendGrip(grips, entity, VpGripRole::RadiusPoint, 2,
                   {circle.center.x - circle.radius, circle.center.y});
        appendGrip(grips, entity, VpGripRole::RadiusPoint, 3,
                   {circle.center.x, circle.center.y - circle.radius});
    }
    else if (entity.type == VpEntityType::Arc)
    {
        const auto& arc = std::get<VpArcEntity>(entity.geometry);
        const double span = normalizedAngle(arc.is_clockwise ? arc.start_angle - arc.end_angle
                                                             : arc.end_angle - arc.start_angle);
        appendGrip(grips, entity, VpGripRole::MoveEntity, 0, arc.center);
        appendGrip(grips, entity, VpGripRole::ArcStartPoint, 0, arcPoint(arc, arc.start_angle));
        appendGrip(grips, entity, VpGripRole::RadiusPoint, 0,
                   arcPoint(arc, arc.start_angle + (arc.is_clockwise ? -span : span) * 0.5));
        appendGrip(grips, entity, VpGripRole::ArcEndPoint, 0, arcPoint(arc, arc.end_angle));
    }
    else if (entity.type == VpEntityType::Polyline)
    {
        const auto& polyline = std::get<VpPolylineEntity>(entity.geometry);
        grips.reserve(polyline.vertices.size());
        for (std::size_t index = 0; index < polyline.vertices.size(); ++index)
        {
            appendGrip(grips, entity, VpGripRole::ControlPoint, index, polyline.vertices[index]);
        }
    }
    else if (entity.type == VpEntityType::Text)
    {
        appendGrip(grips, entity, VpGripRole::MoveEntity, 0,
                   std::get<VpTextEntity>(entity.geometry).position);
    }
    else if (entity.type == VpEntityType::LinearDimension)
    {
        const auto& dimension = std::get<VpLinearDimensionEntity>(entity.geometry);
        appendGrip(grips, entity, VpGripRole::ControlPoint, 0, dimension.first_point);
        appendGrip(grips, entity, VpGripRole::ControlPoint, 1, dimension.second_point);
        appendGrip(grips, entity, VpGripRole::ControlPoint, 2, dimension.dimension_line_point);
        if (dimension.dimension_type == VpDimensionType::Angular ||
            dimension.dimension_type == VpDimensionType::ArcLength ||
            dimension.dimension_type == VpDimensionType::Radius ||
            dimension.dimension_type == VpDimensionType::Diameter ||
            dimension.dimension_type == VpDimensionType::Ordinate)
        {
            appendGrip(grips, entity, VpGripRole::ControlPoint, 3, dimension.center_point);
        }
    }
    else if (entity.type == VpEntityType::Hatch)
    {
        const auto& boundary = std::get<VpHatchEntity>(entity.geometry).boundary;
        grips.reserve(boundary.size());
        for (std::size_t index = 0; index < boundary.size(); ++index)
        {
            appendGrip(grips, entity, VpGripRole::ControlPoint, index, boundary[index]);
        }
    }
    else if (entity.type == VpEntityType::Spline)
    {
        const auto& control_points = std::get<VpSplineEntity>(entity.geometry).control_points;
        for (std::size_t index = 0; index < control_points.size(); ++index)
        {
            appendGrip(grips, entity, VpGripRole::ControlPoint, index, control_points[index]);
        }
    }
    else if (entity.type == VpEntityType::Ellipse)
    {
        const auto& ellipse = std::get<VpEllipseEntity>(entity.geometry);
        appendGrip(grips, entity, VpGripRole::MoveEntity, 0, ellipse.center);
        appendGrip(
            grips, entity, VpGripRole::ControlPoint, 0,
            {ellipse.center.x + ellipse.major_axis.x, ellipse.center.y + ellipse.major_axis.y});
        appendGrip(
            grips, entity, VpGripRole::ControlPoint, 1,
            {ellipse.center.x - ellipse.major_axis.x, ellipse.center.y - ellipse.major_axis.y});
        appendGrip(
            grips, entity, VpGripRole::ControlPoint, 2,
            {ellipse.center.x + ellipse.minor_axis.x, ellipse.center.y + ellipse.minor_axis.y});
        appendGrip(
            grips, entity, VpGripRole::ControlPoint, 3,
            {ellipse.center.x - ellipse.minor_axis.x, ellipse.center.y - ellipse.minor_axis.y});
    }
    else if (entity.type == VpEntityType::MText)
    {
        appendGrip(grips, entity, VpGripRole::MoveEntity, 0,
                   std::get<VpMTextEntity>(entity.geometry).position);
    }
    else if (entity.type == VpEntityType::Leader)
    {
        const auto& vertices = std::get<VpLeaderEntity>(entity.geometry).vertices;
        for (std::size_t index = 0; index < vertices.size(); ++index)
        {
            appendGrip(grips, entity, VpGripRole::ControlPoint, index, vertices[index]);
        }
    }
    return grips;
}

bool gripEditedEntity(const VpEntityRecord& source, const VpGripHandle& grip,
                      const VpPoint2d& destination, VpEntityRecord& result)
{
    if (source.id != grip.entity_id)
    {
        return false;
    }
    if (grip.role == VpGripRole::MoveEntity)
    {
        result =
            translatedEntity(source, destination.x - grip.point.x, destination.y - grip.point.y);
        return true;
    }
    result = source;
    if (source.type == VpEntityType::Line && grip.role == VpGripRole::ControlPoint)
    {
        auto& line = std::get<VpLineEntity>(result.geometry);
        if (grip.index > 1)
        {
            return false;
        }
        (grip.index == 0 ? line.start_point : line.end_point) = destination;
        return distance(line.start_point, line.end_point) > 1.0e-9;
    }
    if (source.type == VpEntityType::Circle && grip.role == VpGripRole::RadiusPoint)
    {
        auto& circle = std::get<VpCircleEntity>(result.geometry);
        circle.radius = distance(circle.center, destination);
        return circle.radius > 1.0e-9;
    }
    if (source.type == VpEntityType::Arc)
    {
        auto& arc = std::get<VpArcEntity>(result.geometry);
        if (grip.role == VpGripRole::RadiusPoint)
        {
            arc.radius = distance(arc.center, destination);
            return arc.radius > 1.0e-9;
        }
        if (grip.role == VpGripRole::ArcStartPoint)
        {
            arc.start_angle = entityAngleDegrees(arc.center, destination);
            return normalizedAngle(arc.is_clockwise ? arc.start_angle - arc.end_angle
                                                    : arc.end_angle - arc.start_angle) > 1.0e-7;
        }
        if (grip.role == VpGripRole::ArcEndPoint)
        {
            arc.end_angle = entityAngleDegrees(arc.center, destination);
            return normalizedAngle(arc.is_clockwise ? arc.start_angle - arc.end_angle
                                                    : arc.end_angle - arc.start_angle) > 1.0e-7;
        }
    }
    if (source.type == VpEntityType::Polyline && grip.role == VpGripRole::ControlPoint)
    {
        auto& polyline = std::get<VpPolylineEntity>(result.geometry);
        if (grip.index >= polyline.vertices.size())
        {
            return false;
        }
        polyline.vertices[grip.index] = destination;
        return true;
    }
    if (source.type == VpEntityType::LinearDimension && grip.role == VpGripRole::ControlPoint)
    {
        auto& dimension = std::get<VpLinearDimensionEntity>(result.geometry);
        if (grip.index == 0)
        {
            dimension.first_point = destination;
        }
        else if (grip.index == 1)
        {
            dimension.second_point = destination;
        }
        else if (grip.index == 2)
        {
            dimension.dimension_line_point = destination;
        }
        else if (grip.index == 3)
        {
            dimension.center_point = destination;
        }
        else
        {
            return false;
        }
        return true;
    }
    if (source.type == VpEntityType::Hatch && grip.role == VpGripRole::ControlPoint)
    {
        auto& boundary = std::get<VpHatchEntity>(result.geometry).boundary;
        if (grip.index >= boundary.size())
        {
            return false;
        }
        boundary[grip.index] = destination;
        return true;
    }
    if (source.type == VpEntityType::Spline && grip.role == VpGripRole::ControlPoint)
    {
        auto& control_points = std::get<VpSplineEntity>(result.geometry).control_points;
        if (grip.index >= control_points.size())
        {
            return false;
        }
        control_points[grip.index] = destination;
        return true;
    }
    if (source.type == VpEntityType::Ellipse && grip.role == VpGripRole::ControlPoint)
    {
        auto& ellipse = std::get<VpEllipseEntity>(result.geometry);
        if (grip.index < 2)
        {
            ellipse.major_axis =
                grip.index == 0
                    ? VpPoint2d{destination.x - ellipse.center.x, destination.y - ellipse.center.y}
                    : VpPoint2d{ellipse.center.x - destination.x, ellipse.center.y - destination.y};
            return std::hypot(ellipse.major_axis.x, ellipse.major_axis.y) > 1.0e-9;
        }
        if (grip.index < 4)
        {
            ellipse.minor_axis =
                grip.index == 2
                    ? VpPoint2d{destination.x - ellipse.center.x, destination.y - ellipse.center.y}
                    : VpPoint2d{ellipse.center.x - destination.x, ellipse.center.y - destination.y};
            return std::hypot(ellipse.minor_axis.x, ellipse.minor_axis.y) > 1.0e-9;
        }
    }
    if (source.type == VpEntityType::Leader && grip.role == VpGripRole::ControlPoint)
    {
        auto& vertices = std::get<VpLeaderEntity>(result.geometry).vertices;
        if (grip.index >= vertices.size())
        {
            return false;
        }
        vertices[grip.index] = destination;
        return true;
    }
    return false;
}

bool VpCadViewport::beginGripDrag(const QPointF& screen_point)
{
    if (m_tool_mode != VpToolMode::Select || !m_document)
    {
        return false;
    }
    double best_distance = 7.0;
    std::optional<VpGripHandle> best_grip;
    for (VpEntityId entity_id : m_selected_entity_ids)
    {
        const VpEntityRecord* entity = entityById(entity_id);
        if (!entity)
        {
            continue;
        }
        for (const VpGripHandle& grip : entityGripHandles(*entity))
        {
            const QPointF grip_screen = worldToScreen(grip.point);
            const double candidate =
                std::hypot(grip_screen.x() - screen_point.x(), grip_screen.y() - screen_point.y());
            if (candidate <= best_distance)
            {
                best_distance = candidate;
                best_grip = grip;
            }
        }
    }
    if (!best_grip)
    {
        return false;
    }
    if (!entityById(best_grip->entity_id))
    {
        return false;
    }
    m_active_grip = best_grip;
    m_grip_sources.clear();
    for (VpEntityId entity_id : m_selected_entity_ids)
    {
        const VpEntityRecord* selected_source = entityById(entity_id);
        if (selected_source)
        {
            m_grip_sources.push_back(*selected_source);
        }
    }
    m_grip_previews = m_grip_sources;
    m_grip_reference_point = selectionGripReference(m_grip_sources, best_grip->point);
    m_grip_has_change = false;
    m_is_grip_dragging = true;
    setCursor(Qt::SizeAllCursor);
    emit commandMessage(tr("夹点%1：指定新位置，Space 循环模式，Esc 取消。")
                            .arg(gripOperationName(m_grip_operation)));
    update();
    return true;
}

void VpCadViewport::updateGripDrag(const VpPoint2d& destination)
{
    if (!m_is_grip_dragging || !m_active_grip || m_grip_sources.empty())
    {
        return;
    }
    if (m_grip_operation == VpGripOperation::Stretch)
    {
        m_grip_previews = m_grip_sources;
        m_grip_has_change = false;
        const auto source_iterator = std::find_if(m_grip_sources.begin(), m_grip_sources.end(),
                                                  [this](const VpEntityRecord& source)
                                                  {
                                                      return source.id == m_active_grip->entity_id;
                                                  });
        if (source_iterator != m_grip_sources.end())
        {
            VpEntityRecord preview;
            if (gripEditedEntity(*source_iterator, *m_active_grip, destination, preview))
            {
                const std::size_t index =
                    static_cast<std::size_t>(source_iterator - m_grip_sources.begin());
                m_grip_previews[index] = std::move(preview);
                m_grip_has_change = distance(m_active_grip->point, destination) > 1.0e-9;
            }
        }
    }
    else
    {
        m_grip_previews =
            gripTransformedEntities(m_grip_sources, m_grip_operation, m_active_grip->point,
                                    m_grip_reference_point, destination);
        const double destination_distance = distance(m_active_grip->point, destination);
        if (m_grip_operation == VpGripOperation::Move)
        {
            m_grip_has_change = destination_distance > 1.0e-9;
        }
        else if (m_grip_operation == VpGripOperation::Rotate)
        {
            const double reference_angle =
                entityAngleDegrees(m_active_grip->point, m_grip_reference_point);
            const double destination_angle = entityAngleDegrees(m_active_grip->point, destination);
            m_grip_has_change =
                destination_distance > 1.0e-9 &&
                std::abs(std::remainder(destination_angle - reference_angle, 360.0)) > 1.0e-9;
        }
        else if (m_grip_operation == VpGripOperation::Scale)
        {
            const double reference_distance =
                distance(m_active_grip->point, m_grip_reference_point);
            m_grip_has_change =
                reference_distance > 1.0e-9 && destination_distance > 1.0e-9 &&
                std::abs((destination_distance / reference_distance) - 1.0) > 1.0e-9;
        }
        else
        {
            m_grip_has_change = destination_distance > 1.0e-9;
        }
    }
    update();
}

void VpCadViewport::commitGripDrag()
{
    if (m_document && m_is_grip_dragging && m_active_grip && m_grip_has_change &&
        !m_grip_previews.empty())
    {
        auto transaction =
            m_document->beginTransaction(tr("夹点%1").arg(gripOperationName(m_grip_operation)));
        if (m_grip_operation == VpGripOperation::Stretch)
        {
            const auto preview_iterator =
                std::find_if(m_grip_previews.begin(), m_grip_previews.end(),
                             [this](const VpEntityRecord& preview)
                             {
                                 return preview.id == m_active_grip->entity_id;
                             });
            if (preview_iterator != m_grip_previews.end())
            {
                transaction->replaceEntity(preview_iterator->id, *preview_iterator);
            }
        }
        else
        {
            for (const VpEntityRecord& preview : m_grip_previews)
            {
                transaction->replaceEntity(preview.id, preview);
            }
        }
        transaction->commit();
        emit commandMessage(
            tr("夹点%1完成。可继续拖动其他夹点。").arg(gripOperationName(m_grip_operation)));
    }
    m_active_grip.reset();
    m_grip_sources.clear();
    m_grip_previews.clear();
    m_is_grip_dragging = false;
    m_grip_has_change = false;
    setCursor(Qt::CrossCursor);
    update();
}

void VpCadViewport::cancelGripDrag()
{
    m_active_grip.reset();
    m_grip_sources.clear();
    m_grip_previews.clear();
    m_is_grip_dragging = false;
    m_grip_has_change = false;
    setCursor(Qt::CrossCursor);
    update();
}

void VpCadViewport::drawGrips(QPainter& painter)
{
    if (m_tool_mode != VpToolMode::Select)
    {
        return;
    }
    painter.save();
    if (m_is_grip_dragging && !m_grip_previews.empty())
    {
        painter.setPen(QPen(QColor(92, 214, 255), 1.8, Qt::DashLine));
        for (const VpEntityRecord& preview : m_grip_previews)
        {
            drawEntityGeometry(painter, preview, QColor(92, 214, 255, 48));
        }
        if (m_active_grip && m_grip_operation != VpGripOperation::Stretch)
        {
            painter.setPen(QPen(QColor(92, 214, 255, 150), 1.0, Qt::DashLine));
            painter.drawLine(worldToScreen(m_active_grip->point),
                             worldToScreen(m_grip_reference_point));
            painter.drawLine(worldToScreen(m_active_grip->point), worldToScreen(m_cursor_world));
        }
    }
    for (VpEntityId entity_id : m_selected_entity_ids)
    {
        const VpEntityRecord* entity = entityById(entity_id);
        if (!entity)
        {
            continue;
        }
        for (const VpGripHandle& grip : entityGripHandles(*entity))
        {
            const QPointF center = worldToScreen(grip.point);
            const bool is_active = m_active_grip && m_active_grip->entity_id == grip.entity_id &&
                                   m_active_grip->role == grip.role &&
                                   m_active_grip->index == grip.index;
            const double size = is_active ? 9.0 : 7.0;
            painter.setPen(QPen(is_active ? QColor(255, 255, 255) : QColor(14, 67, 96), 1.0));
            painter.setBrush(is_active ? QColor(226, 80, 80) : QColor(76, 190, 235));
            painter.drawRect(
                QRectF(center.x() - (size * 0.5), center.y() - (size * 0.5), size, size));
        }
    }
    painter.restore();
}

} // namespace Vp
