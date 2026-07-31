#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_viewport_geometry.h"
#include "s_dimension_geometry.h"
#include "s_document_transaction.h"
#include "s_ellipse_geometry.h"
#include "s_entity.h"
#include "s_hatch_geometry.h"
#include "s_spline_geometry.h"

#include <QKeyEvent>
#include <QKeySequence>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace smartCam
{
namespace
{

double pointToSegmentDistance(const SPoint2d& point, const SPoint2d& segment_start,
                              const SPoint2d& segment_end)
{
    const double delta_x = segment_end.x - segment_start.x;
    const double delta_y = segment_end.y - segment_start.y;
    const double length_squared = (delta_x * delta_x) + (delta_y * delta_y);
    if (length_squared <= 1.0e-18)
    {
        return distance(point, segment_start);
    }
    const double projection = std::clamp(
        (((point.x - segment_start.x) * delta_x) + ((point.y - segment_start.y) * delta_y)) /
            length_squared,
        0.0, 1.0);
    return distance(point, {segment_start.x + (projection * delta_x),
                            segment_start.y + (projection * delta_y)});
}

double normalizedAngle(double angle)
{
    const double result = std::fmod(angle, 360.0);
    return result < 0.0 ? result + 360.0 : result;
}

bool angleOnArc(double angle, const SArcEntity& arc)
{
    const double total_span = normalizedAngle(arc.is_clockwise ? arc.start_angle - arc.end_angle
                                                                : arc.end_angle - arc.start_angle);
    const double point_span = normalizedAngle(arc.is_clockwise ? arc.start_angle - angle
                                                                : angle - arc.start_angle);
    return point_span <= total_span + 1.0e-7;
}

} // namespace

SPoint2d SCadViewport::constrainedPoint(const SPoint2d& world_point)
{
    SPoint2d result = world_point;
    m_active_object_snap.reset();
    const bool is_stretch_window = m_tool_mode == SToolMode::Stretch && m_input_points.size() < 2;
    const std::optional<SPoint2d> reference =
        is_stretch_window ? std::optional<SPoint2d>() : coordinateReferencePoint();
    if (!is_stretch_window && m_is_object_snap_enabled && m_document)
    {
        const double snap_tolerance = 10.0 / m_zoom;
        std::vector<const SEntityRecord*> visible_entities;
        visible_entities.reserve(m_document->entities().size());
        for (const SEntityRecord& entity : m_document->entities())
        {
            if (m_document->isLayerVisible(entity.layer_name))
            {
                visible_entities.push_back(&entity);
            }
        }
        const std::optional<SObjectSnapResult> direct_snap =
            findObjectSnap(visible_entities, world_point, reference, snap_tolerance,
                           m_object_snap_modes);
        const bool can_acquire = direct_snap.has_value();
        if (can_acquire)
        {
            const bool already_acquired =
                std::any_of(m_tracking_points.begin(), m_tracking_points.end(),
                            [&](const SPoint2d& point)
                            {
                                return distance(point, direct_snap->point) <= 1.0e-9;
                            });
            if (!already_acquired)
            {
                if (m_tracking_points.size() >= 8)
                {
                    m_tracking_points.erase(m_tracking_points.begin());
                }
                m_tracking_points.push_back(direct_snap->point);
            }
            m_active_object_snap = direct_snap;
            return m_active_object_snap->point;
        }
        m_active_object_snap = trackingSnap(world_point, reference, snap_tolerance);
        if (m_active_object_snap)
        {
            return m_active_object_snap->point;
        }
        m_active_object_snap = direct_snap;
        if (m_active_object_snap)
        {
            return m_active_object_snap->point;
        }
    }

    if (m_is_grid_snap_enabled && m_grid_snap_spacing > 1.0e-9)
    {
        result = snapPointToDraftingGrid(result, m_grid_snap_spacing,
                                         draftingGridBasis(m_grid_rotation));
    }

    if (m_is_ortho_enabled && reference)
    {
        const double delta_x = std::abs(result.x - reference->x);
        const double delta_y = std::abs(result.y - reference->y);
        if (delta_x >= delta_y)
        {
            result.y = reference->y;
        }
        else
        {
            result.x = reference->x;
        }
    }
    return result;
}

std::optional<SEntityId> SCadViewport::entityAt(const SPoint2d& world_point) const
{
    if (!m_document)
    {
        return std::nullopt;
    }
    const double tolerance = 8.0 / m_zoom;
    double best_distance = tolerance;
    std::optional<SEntityId> best_id;
    for (const SEntityRecord& entity : m_document->entities())
    {
        if (!m_document->isLayerVisible(entity.layer_name) ||
            m_document->isLayerLocked(entity.layer_name))
        {
            continue;
        }
        double entity_distance = std::numeric_limits<double>::max();
        if (entity.type == SEntityType::Line)
        {
            const auto& line = std::get<SLineEntity>(entity.geometry);
            entity_distance = pointToSegmentDistance(world_point, line.start_point, line.end_point);
        }
        else if (entity.type == SEntityType::Circle)
        {
            const auto& circle = std::get<SCircleEntity>(entity.geometry);
            entity_distance = std::abs(distance(world_point, circle.center) - circle.radius);
        }
        else if (entity.type == SEntityType::Arc)
        {
            const auto& arc = std::get<SArcEntity>(entity.geometry);
            if (angleOnArc(entityAngleDegrees(arc.center, world_point), arc))
            {
                entity_distance = std::abs(distance(world_point, arc.center) - arc.radius);
            }
        }
        else if (entity.type == SEntityType::Polyline)
        {
            const auto& polyline = std::get<SPolylineEntity>(entity.geometry);
            const std::size_t segment_count =
                polyline.vertices.size() < 2 ? 0
                                             : (polyline.is_closed ? polyline.vertices.size()
                                                                   : polyline.vertices.size() - 1);
            for (std::size_t index = 0; index < segment_count; ++index)
            {
                const std::size_t end_index = (index + 1) % polyline.vertices.size();
                SArcEntity arc;
                if (index < polyline.bulges.size() &&
                    bulgeArc(polyline.vertices[index], polyline.vertices[end_index],
                             polyline.bulges[index], arc) &&
                    angleOnArc(entityAngleDegrees(arc.center, world_point), arc))
                {
                    entity_distance = std::min(
                        entity_distance, std::abs(distance(world_point, arc.center) - arc.radius));
                }
                else
                {
                    entity_distance =
                        std::min(entity_distance,
                                 pointToSegmentDistance(world_point, polyline.vertices[index],
                                                        polyline.vertices[end_index]));
                }
            }
            const double maximum_width = std::max(
                polyline.start_widths.empty()
                    ? 0.0
                    : *std::max_element(polyline.start_widths.begin(), polyline.start_widths.end()),
                polyline.end_widths.empty()
                    ? 0.0
                    : *std::max_element(polyline.end_widths.begin(), polyline.end_widths.end()));
            entity_distance = std::max(0.0, entity_distance - (maximum_width * 0.5));
        }
        else if (entity.type == SEntityType::Text)
        {
            entity_distance =
                distance(world_point, std::get<STextEntity>(entity.geometry).position);
        }
        else if (entity.type == SEntityType::LinearDimension)
        {
            const auto& dimension = std::get<SLinearDimensionEntity>(entity.geometry);
            const std::vector<SPoint2d> points = dimensionReferencePoints(dimension);
            for (std::size_t index = 1; index < points.size(); ++index)
            {
                entity_distance =
                    std::min(entity_distance,
                             pointToSegmentDistance(world_point, points[index - 1], points[index]));
            }
        }
        else if (entity.type == SEntityType::Hatch)
        {
            const SHatchEntity& hatch =
                std::get<SHatchEntity>(entity.geometry);
            const auto& boundary = hatch.boundary;
            bool is_inside = pointInsidePolygon(world_point, boundary);
            for (const std::vector<SPoint2d>& island : hatch.island_boundaries)
            {
                if (pointInsidePolygon(world_point, island))
                {
                    is_inside = false;
                    break;
                }
            }
            if (is_inside)
            {
                entity_distance = 0.0;
            }
            for (std::size_t index = 1; index < boundary.size(); ++index)
            {
                entity_distance = std::min(
                    entity_distance,
                    pointToSegmentDistance(world_point, boundary[index - 1], boundary[index]));
            }
            if (boundary.size() > 2)
            {
                entity_distance =
                    std::min(entity_distance, pointToSegmentDistance(world_point, boundary.back(),
                                                                     boundary.front()));
            }
        }
        else if (entity.type == SEntityType::Spline)
        {
            const std::vector<SPoint2d> points =
                splineApproximation(std::get<SSplineEntity>(entity.geometry));
            for (std::size_t index = 1; index < points.size(); ++index)
            {
                entity_distance =
                    std::min(entity_distance,
                             pointToSegmentDistance(world_point, points[index - 1], points[index]));
            }
        }
        else if (entity.type == SEntityType::Ellipse)
        {
            const std::vector<SPoint2d> points =
                ellipseApproximation(std::get<SEllipseEntity>(entity.geometry));
            for (std::size_t index = 1; index < points.size(); ++index)
            {
                entity_distance =
                    std::min(entity_distance,
                             pointToSegmentDistance(world_point, points[index - 1], points[index]));
            }
        }
        else if (entity.type == SEntityType::MText)
        {
            entity_distance =
                distance(world_point, std::get<SMTextEntity>(entity.geometry).position);
        }
        else if (entity.type == SEntityType::Leader)
        {
            const auto& vertices = std::get<SLeaderEntity>(entity.geometry).vertices;
            for (std::size_t index = 1; index < vertices.size(); ++index)
            {
                entity_distance = std::min(
                    entity_distance,
                    pointToSegmentDistance(world_point, vertices[index - 1], vertices[index]));
            }
        }
        if (entity_distance <= best_distance)
        {
            best_distance = entity_distance;
            best_id = entity.id;
        }
    }
    return best_id;
}

const SEntityRecord* SCadViewport::entityById(SEntityId entity_id) const
{
    if (!m_document)
    {
        return nullptr;
    }
    const auto iterator = std::find_if(m_document->entities().begin(), m_document->entities().end(),
                                       [entity_id](const SEntityRecord& entity)
                                       {
                                           return entity.id == entity_id;
                                       });
    return iterator == m_document->entities().end() ? nullptr : &*iterator;
}

void SCadViewport::selectAt(const SPoint2d& world_point)
{
    const std::optional<SEntityId> picked_id = entityAt(world_point);
    if (!m_is_additive_selection)
    {
        m_selected_entity_ids.clear();
    }
    if (picked_id)
    {
        std::vector<SEntityId> picked_ids{*picked_id};
        const SEntityRecord* picked = entityById(*picked_id);
        if (picked && picked->associative_array)
        {
            picked_ids.clear();
            const SArrayId array_id = picked->associative_array->array_id;
            for (const SEntityRecord& entity : m_document->entities())
            {
                if (entity.associative_array && entity.associative_array->array_id == array_id)
                {
                    picked_ids.push_back(entity.id);
                }
            }
        }
        const bool is_group_selected = std::all_of(
            picked_ids.begin(), picked_ids.end(),
            [this](SEntityId entity_id)
            {
                return std::find(m_selected_entity_ids.begin(), m_selected_entity_ids.end(),
                                 entity_id) != m_selected_entity_ids.end();
            });
        if (m_is_additive_selection && is_group_selected)
        {
            for (SEntityId entity_id : picked_ids)
            {
                const auto iterator = std::find(m_selected_entity_ids.begin(),
                                                m_selected_entity_ids.end(), entity_id);
                if (iterator != m_selected_entity_ids.end())
                {
                    m_selected_entity_ids.erase(iterator);
                }
            }
        }
        else
        {
            for (SEntityId entity_id : picked_ids)
            {
                if (std::find(m_selected_entity_ids.begin(), m_selected_entity_ids.end(),
                              entity_id) == m_selected_entity_ids.end())
                {
                    m_selected_entity_ids.push_back(entity_id);
                }
            }
        }
    }
    m_selected_entity_id = m_selected_entity_ids.empty()
                               ? std::optional<SEntityId>()
                               : std::optional<SEntityId>(m_selected_entity_ids.front());
    emitSelectionState();
    emit commandMessage(m_selected_entity_ids.empty()
                            ? tr("未选择对象。")
                            : tr("已选择 %1 个实体。").arg(m_selected_entity_ids.size()));
    update();
}

void SCadViewport::selectInWindow(const SPoint2d& first_corner, const SPoint2d& second_corner,
                                  bool is_crossing)
{
    if (!m_document)
    {
        return;
    }
    const QRectF selection_rectangle(QPointF(std::min(first_corner.x, second_corner.x),
                                             std::min(first_corner.y, second_corner.y)),
                                     QPointF(std::max(first_corner.x, second_corner.x),
                                             std::max(first_corner.y, second_corner.y)));
    if (!m_is_additive_selection)
    {
        m_selected_entity_ids.clear();
    }
    for (const SEntityRecord& entity : m_document->entities())
    {
        if (!m_document->isLayerVisible(entity.layer_name) ||
            m_document->isLayerLocked(entity.layer_name))
        {
            continue;
        }
        const QRectF bounds = entityBounds(entity);
        const bool is_match = is_crossing ? selection_rectangle.intersects(bounds)
                                          : selection_rectangle.contains(bounds);
        if (is_match && std::find(m_selected_entity_ids.begin(), m_selected_entity_ids.end(),
                                  entity.id) == m_selected_entity_ids.end())
        {
            m_selected_entity_ids.push_back(entity.id);
        }
    }
    m_selected_entity_id = m_selected_entity_ids.empty()
                               ? std::optional<SEntityId>()
                               : std::optional<SEntityId>(m_selected_entity_ids.front());
    emitSelectionState();
    emit commandMessage(tr("%1框选：已选择 %2 个实体。")
                            .arg(is_crossing ? tr("交叉") : tr("窗口"))
                            .arg(m_selected_entity_ids.size()));
}

QRectF SCadViewport::entityBounds(const SEntityRecord& entity) const
{
    double minimum_x = std::numeric_limits<double>::max();
    double minimum_y = std::numeric_limits<double>::max();
    double maximum_x = std::numeric_limits<double>::lowest();
    double maximum_y = std::numeric_limits<double>::lowest();
    double bounds_padding = 0.0;
    const auto include_point =
        [&minimum_x, &minimum_y, &maximum_x, &maximum_y](const SPoint2d& point)
    {
        minimum_x = std::min(minimum_x, point.x);
        minimum_y = std::min(minimum_y, point.y);
        maximum_x = std::max(maximum_x, point.x);
        maximum_y = std::max(maximum_y, point.y);
    };
    if (entity.type == SEntityType::Line)
    {
        const auto& line = std::get<SLineEntity>(entity.geometry);
        include_point(line.start_point);
        include_point(line.end_point);
    }
    else if (entity.type == SEntityType::Circle)
    {
        const auto& circle = std::get<SCircleEntity>(entity.geometry);
        include_point({circle.center.x - circle.radius, circle.center.y - circle.radius});
        include_point({circle.center.x + circle.radius, circle.center.y + circle.radius});
    }
    else if (entity.type == SEntityType::Arc)
    {
        const auto& arc = std::get<SArcEntity>(entity.geometry);
        include_point({arc.center.x - arc.radius, arc.center.y - arc.radius});
        include_point({arc.center.x + arc.radius, arc.center.y + arc.radius});
    }
    else if (entity.type == SEntityType::Polyline)
    {
        const auto& polyline = std::get<SPolylineEntity>(entity.geometry);
        for (const SPoint2d& vertex : polyline.vertices)
        {
            include_point(vertex);
        }
        bounds_padding =
            std::max(
                polyline.start_widths.empty()
                    ? 0.0
                    : *std::max_element(polyline.start_widths.begin(), polyline.start_widths.end()),
                polyline.end_widths.empty()
                    ? 0.0
                    : *std::max_element(polyline.end_widths.begin(), polyline.end_widths.end())) *
            0.5;
    }
    else if (entity.type == SEntityType::Text)
    {
        const auto& text = std::get<STextEntity>(entity.geometry);
        include_point(text.position);
        include_point(
            {text.position.x + (text.height * text.text.size()), text.position.y + text.height});
    }
    else if (entity.type == SEntityType::LinearDimension)
    {
        for (const SPoint2d& point :
             dimensionReferencePoints(std::get<SLinearDimensionEntity>(entity.geometry)))
        {
            include_point(point);
        }
    }
    else if (entity.type == SEntityType::Hatch)
    {
        for (const SPoint2d& vertex : std::get<SHatchEntity>(entity.geometry).boundary)
        {
            include_point(vertex);
        }
    }
    else if (entity.type == SEntityType::Spline)
    {
        for (const SPoint2d& control_point :
             std::get<SSplineEntity>(entity.geometry).control_points)
        {
            include_point(control_point);
        }
    }
    else if (entity.type == SEntityType::Ellipse)
    {
        for (const SPoint2d& point :
             ellipseApproximation(std::get<SEllipseEntity>(entity.geometry)))
        {
            include_point(point);
        }
    }
    else if (entity.type == SEntityType::MText)
    {
        const auto& text = std::get<SMTextEntity>(entity.geometry);
        include_point(text.position);
        include_point({text.position.x + text.width, text.position.y + (text.height * 3.0)});
    }
    else if (entity.type == SEntityType::Leader)
    {
        for (const SPoint2d& point : std::get<SLeaderEntity>(entity.geometry).vertices)
        {
            include_point(point);
        }
    }
    if (minimum_x > maximum_x || minimum_y > maximum_y)
    {
        return {};
    }
    minimum_x -= bounds_padding;
    minimum_y -= bounds_padding;
    maximum_x += bounds_padding;
    maximum_y += bounds_padding;
    const double minimum_extent = 1.0 / std::max(m_zoom, 1.0e-9);
    if ((maximum_x - minimum_x) < minimum_extent)
    {
        maximum_x += minimum_extent;
    }
    if ((maximum_y - minimum_y) < minimum_extent)
    {
        maximum_y += minimum_extent;
    }
    return QRectF(QPointF(minimum_x, minimum_y), QPointF(maximum_x, maximum_y));
}

void SCadViewport::emitSelectionState()
{
    emit selectedEntityChanged(m_selected_entity_ids.size() == 1
                                   ? static_cast<quint64>(m_selected_entity_ids.front())
                                   : 0);
    emit selectionChanged(selectedEntityIds());
}

void SCadViewport::setSelectedEntityIds(const std::vector<SEntityId>& entity_ids)
{
    m_selected_entity_ids.clear();
    for (SEntityId entity_id : entity_ids)
    {
        if (entityById(entity_id))
        {
            m_selected_entity_ids.push_back(entity_id);
        }
    }
    m_selected_entity_id = m_selected_entity_ids.empty()
                               ? std::optional<SEntityId>()
                               : std::optional<SEntityId>(m_selected_entity_ids.front());
    emitSelectionState();
    update();
}

std::optional<SPoint2d> SCadViewport::entitySetBoundsCenter(
    const std::vector<SEntityId>& entity_ids) const
{
    QRectF combined_bounds;
    bool has_bounds = false;
    for (SEntityId entity_id : entity_ids)
    {
        const SEntityRecord* entity = entityById(entity_id);
        if (!entity)
        {
            continue;
        }
        const QRectF bounds = entityBounds(*entity);
        if (!bounds.isValid())
        {
            continue;
        }
        combined_bounds = has_bounds ? combined_bounds.united(bounds) : bounds;
        has_bounds = true;
    }
    if (!has_bounds)
    {
        return std::nullopt;
    }
    return SPoint2d{combined_bounds.center().x(), combined_bounds.center().y()};
}

double SCadViewport::adaptiveGridSpacing() const
{
    double spacing = m_grid_snap_spacing;
    while ((spacing * m_zoom) < 24.0)
    {
        spacing *= 2.0;
    }
    while ((spacing * m_zoom) > 96.0)
    {
        spacing *= 0.5;
    }
    return spacing;
}

} // namespace smartCam
