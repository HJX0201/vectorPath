#include "vp_cad_viewport.h"
#include "vp_cad_viewport_geometry.h"

#include <QPainter>
#include <algorithm>
#include <cmath>
#include <limits>

namespace Vp
{
namespace
{

constexpr double kPi = 3.14159265358979323846;

double normalizedAngle(double angle)
{
    double result = std::fmod(angle, 360.0);
    return result < 0.0 ? result + 360.0 : result;
}

double counterClockwiseSpan(double start_angle, double end_angle)
{
    return normalizedAngle(end_angle - start_angle);
}

bool angleOnArc(double angle, const VpArcEntity& arc)
{
    const double total_span = counterClockwiseSpan(arc.start_angle, arc.end_angle);
    const double point_span = counterClockwiseSpan(arc.start_angle, angle);
    return point_span <= total_span + 1.0e-7;
}

std::vector<VpPoint2d> lineCircleIntersections(const VpLineEntity& line, const VpPoint2d& center,
                                               double radius)
{
    std::vector<VpPoint2d> points;
    const double delta_x = line.end_point.x - line.start_point.x;
    const double delta_y = line.end_point.y - line.start_point.y;
    const double origin_x = line.start_point.x - center.x;
    const double origin_y = line.start_point.y - center.y;
    const double coefficient_a = (delta_x * delta_x) + (delta_y * delta_y);
    const double coefficient_b = 2.0 * ((origin_x * delta_x) + (origin_y * delta_y));
    const double coefficient_c = (origin_x * origin_x) + (origin_y * origin_y) - (radius * radius);
    const double discriminant =
        (coefficient_b * coefficient_b) - (4.0 * coefficient_a * coefficient_c);
    if (coefficient_a <= 1.0e-18 || discriminant < -1.0e-9)
    {
        return points;
    }
    const double root = std::sqrt(std::max(0.0, discriminant));
    for (double parameter : {(-coefficient_b - root) / (2.0 * coefficient_a),
                             (-coefficient_b + root) / (2.0 * coefficient_a)})
    {
        if (parameter >= -1.0e-9 && parameter <= 1.0 + 1.0e-9)
        {
            const VpPoint2d point{line.start_point.x + (parameter * delta_x),
                                  line.start_point.y + (parameter * delta_y)};
            if (points.empty() || distance(points.front(), point) > 1.0e-8)
            {
                points.push_back(point);
            }
        }
    }
    return points;
}

std::vector<VpPoint2d> circleCircleIntersections(const VpPoint2d& first_center, double first_radius,
                                                 const VpPoint2d& second_center,
                                                 double second_radius)
{
    std::vector<VpPoint2d> points;
    const double center_distance = distance(first_center, second_center);
    if (center_distance <= 1.0e-12 || center_distance > first_radius + second_radius + 1.0e-9 ||
        center_distance < std::abs(first_radius - second_radius) - 1.0e-9)
    {
        return points;
    }
    const double along = ((first_radius * first_radius) - (second_radius * second_radius) +
                          (center_distance * center_distance)) /
                         (2.0 * center_distance);
    const double height_squared = (first_radius * first_radius) - (along * along);
    if (height_squared < -1.0e-9)
    {
        return points;
    }
    const double unit_x = (second_center.x - first_center.x) / center_distance;
    const double unit_y = (second_center.y - first_center.y) / center_distance;
    const VpPoint2d base{first_center.x + (along * unit_x), first_center.y + (along * unit_y)};
    const double height = std::sqrt(std::max(0.0, height_squared));
    points.push_back({base.x - (height * unit_y), base.y + (height * unit_x)});
    if (height > 1.0e-8)
    {
        points.push_back({base.x + (height * unit_y), base.y - (height * unit_x)});
    }
    return points;
}

bool pointOnEntity(const VpPoint2d& point, const VpEntityRecord& entity)
{
    if (entity.type != VpEntityType::Arc)
    {
        return true;
    }
    const auto& arc = std::get<VpArcEntity>(entity.geometry);
    return angleOnArc(entityAngleDegrees(arc.center, point), arc);
}

std::vector<VpPoint2d> curveIntersections(const VpEntityRecord& first, const VpEntityRecord& second)
{
    std::vector<VpPoint2d> points;
    if (first.type == VpEntityType::Line &&
        (second.type == VpEntityType::Circle || second.type == VpEntityType::Arc))
    {
        const VpPoint2d center = second.type == VpEntityType::Circle
                                     ? std::get<VpCircleEntity>(second.geometry).center
                                     : std::get<VpArcEntity>(second.geometry).center;
        const double radius = second.type == VpEntityType::Circle
                                  ? std::get<VpCircleEntity>(second.geometry).radius
                                  : std::get<VpArcEntity>(second.geometry).radius;
        points = lineCircleIntersections(std::get<VpLineEntity>(first.geometry), center, radius);
    }
    else if (second.type == VpEntityType::Line &&
             (first.type == VpEntityType::Circle || first.type == VpEntityType::Arc))
    {
        points = curveIntersections(second, first);
    }
    else if (first.type != VpEntityType::Line && second.type != VpEntityType::Line)
    {
        const VpPoint2d first_center = first.type == VpEntityType::Circle
                                           ? std::get<VpCircleEntity>(first.geometry).center
                                           : std::get<VpArcEntity>(first.geometry).center;
        const double first_radius = first.type == VpEntityType::Circle
                                        ? std::get<VpCircleEntity>(first.geometry).radius
                                        : std::get<VpArcEntity>(first.geometry).radius;
        const VpPoint2d second_center = second.type == VpEntityType::Circle
                                            ? std::get<VpCircleEntity>(second.geometry).center
                                            : std::get<VpArcEntity>(second.geometry).center;
        const double second_radius = second.type == VpEntityType::Circle
                                         ? std::get<VpCircleEntity>(second.geometry).radius
                                         : std::get<VpArcEntity>(second.geometry).radius;
        points =
            circleCircleIntersections(first_center, first_radius, second_center, second_radius);
    }
    points.erase(std::remove_if(points.begin(), points.end(),
                                [&](const VpPoint2d& point)
                                {
                                    return !pointOnEntity(point, first) ||
                                           !pointOnEntity(point, second);
                                }),
                 points.end());
    return points;
}

std::vector<VpEntityRecord> trimLine(const VpEntityRecord& target,
                                     const std::vector<VpPoint2d>& intersections,
                                     const VpPoint2d& pick_point)
{
    const auto& line = std::get<VpLineEntity>(target.geometry);
    const double delta_x = line.end_point.x - line.start_point.x;
    const double delta_y = line.end_point.y - line.start_point.y;
    const double length_squared = (delta_x * delta_x) + (delta_y * delta_y);
    std::vector<double> boundaries{0.0, 1.0};
    for (const VpPoint2d& point : intersections)
    {
        const double parameter = (((point.x - line.start_point.x) * delta_x) +
                                  ((point.y - line.start_point.y) * delta_y)) /
                                 length_squared;
        if (parameter > 1.0e-8 && parameter < 1.0 - 1.0e-8)
        {
            boundaries.push_back(parameter);
        }
    }
    std::sort(boundaries.begin(), boundaries.end());
    boundaries.erase(std::unique(boundaries.begin(), boundaries.end(),
                                 [](double first, double second)
                                 {
                                     return std::abs(first - second) <= 1.0e-8;
                                 }),
                     boundaries.end());
    if (boundaries.size() <= 2)
    {
        return {};
    }
    const double pick_parameter = std::clamp((((pick_point.x - line.start_point.x) * delta_x) +
                                              ((pick_point.y - line.start_point.y) * delta_y)) /
                                                 length_squared,
                                             0.0, 1.0);
    std::size_t removed_interval = 0;
    while (removed_interval + 1 < boundaries.size() &&
           pick_parameter > boundaries[removed_interval + 1])
    {
        ++removed_interval;
    }
    std::vector<VpEntityRecord> results;
    for (std::size_t index = 0; index + 1 < boundaries.size(); ++index)
    {
        if (index == removed_interval)
        {
            continue;
        }
        const auto point_at = [&](double parameter)
        {
            return VpPoint2d{line.start_point.x + (parameter * delta_x),
                             line.start_point.y + (parameter * delta_y)};
        };
        VpEntityRecord result = target;
        result.geometry =
            VpLineEntity{point_at(boundaries[index]), point_at(boundaries[index + 1])};
        results.push_back(std::move(result));
    }
    return results;
}

double pointToSegmentDistance(const VpPoint2d& point, const VpPoint2d& start_point,
                              const VpPoint2d& end_point)
{
    const double delta_x = end_point.x - start_point.x;
    const double delta_y = end_point.y - start_point.y;
    const double length_squared = (delta_x * delta_x) + (delta_y * delta_y);
    if (length_squared <= 1.0e-18)
    {
        return distance(point, start_point);
    }
    const double parameter =
        std::clamp((((point.x - start_point.x) * delta_x) + ((point.y - start_point.y) * delta_y)) /
                       length_squared,
                   0.0, 1.0);
    return distance(point,
                    {start_point.x + (parameter * delta_x), start_point.y + (parameter * delta_y)});
}

double clockwiseSpan(double start_angle, double end_angle)
{
    return normalizedAngle(start_angle - end_angle);
}

double polylineSegmentParameter(const VpPoint2d& start_point, const VpPoint2d& end_point,
                                double bulge, const VpPoint2d& point)
{
    if (std::abs(bulge) <= 1.0e-12)
    {
        const double delta_x = end_point.x - start_point.x;
        const double delta_y = end_point.y - start_point.y;
        const double length_squared = (delta_x * delta_x) + (delta_y * delta_y);
        return length_squared <= 1.0e-18 ? 0.0
                                         : (((point.x - start_point.x) * delta_x) +
                                            ((point.y - start_point.y) * delta_y)) /
                                               length_squared;
    }
    VpArcEntity arc;
    if (!bulgeArc(start_point, end_point, bulge, arc))
    {
        return 0.0;
    }
    const double start_angle = entityAngleDegrees(arc.center, start_point);
    const double point_angle = entityAngleDegrees(arc.center, point);
    const double total_span = std::abs(4.0 * std::atan(bulge)) * 180.0 / kPi;
    const double point_span = bulge > 0.0 ? counterClockwiseSpan(start_angle, point_angle)
                                          : clockwiseSpan(start_angle, point_angle);
    return total_span <= 1.0e-12 ? 0.0 : point_span / total_span;
}

VpPoint2d polylineSegmentPoint(const VpPoint2d& start_point, const VpPoint2d& end_point,
                               double bulge, double parameter)
{
    if (std::abs(bulge) <= 1.0e-12)
    {
        return {start_point.x + ((end_point.x - start_point.x) * parameter),
                start_point.y + ((end_point.y - start_point.y) * parameter)};
    }
    VpArcEntity arc;
    if (!bulgeArc(start_point, end_point, bulge, arc))
    {
        return start_point;
    }
    const double start_angle = entityAngleDegrees(arc.center, start_point) * kPi / 180.0;
    const double sweep = 4.0 * std::atan(bulge);
    const double angle = start_angle + (sweep * parameter);
    return {arc.center.x + (arc.radius * std::cos(angle)),
            arc.center.y + (arc.radius * std::sin(angle))};
}

double pointToPolylineSegmentDistance(const VpPoint2d& point, const VpPoint2d& start_point,
                                      const VpPoint2d& end_point, double bulge)
{
    if (std::abs(bulge) <= 1.0e-12)
    {
        return pointToSegmentDistance(point, start_point, end_point);
    }
    const double parameter = polylineSegmentParameter(start_point, end_point, bulge, point);
    if (parameter >= 0.0 && parameter <= 1.0)
    {
        VpArcEntity arc;
        if (bulgeArc(start_point, end_point, bulge, arc))
        {
            return std::abs(distance(point, arc.center) - arc.radius);
        }
    }
    return std::min(distance(point, start_point), distance(point, end_point));
}

double partialBulge(double bulge, double start_parameter, double end_parameter)
{
    return std::tan(std::atan(bulge) * std::max(0.0, end_parameter - start_parameter));
}

void appendPolylineEdge(std::vector<VpPoint2d>& vertices, std::vector<double>& bulges,
                        const VpPoint2d& start_point, const VpPoint2d& end_point, double bulge)
{
    if (distance(start_point, end_point) <= 1.0e-9)
    {
        return;
    }
    if (vertices.empty())
    {
        vertices.push_back(start_point);
        bulges.push_back(0.0);
    }
    else if (distance(vertices.back(), start_point) > 1.0e-8)
    {
        return;
    }
    bulges.back() = bulge;
    vertices.push_back(end_point);
    bulges.push_back(0.0);
}

void appendOpenPolylinePart(const VpEntityRecord& source, std::vector<VpPoint2d> vertices,
                            std::vector<double> bulges, std::vector<VpEntityRecord>& results)
{
    if (vertices.size() < 2 || vertices.size() != bulges.size())
    {
        return;
    }
    VpEntityRecord result = source;
    result.geometry = VpPolylineEntity{std::move(vertices), false, std::move(bulges)};
    results.push_back(std::move(result));
}

std::vector<VpEntityRecord> trimOpenPolyline(const VpEntityRecord& cutting_entity,
                                             const VpEntityRecord& target_entity,
                                             const VpPoint2d& pick_point)
{
    const auto& polyline = std::get<VpPolylineEntity>(target_entity.geometry);
    if (polyline.vertices.size() < 2)
    {
        return {};
    }
    std::size_t picked_segment = 0;
    double best_distance = std::numeric_limits<double>::max();
    const std::size_t segment_count =
        polyline.is_closed ? polyline.vertices.size() : polyline.vertices.size() - 1;
    for (std::size_t index = 0; index < segment_count; ++index)
    {
        const std::size_t end_index = (index + 1) % polyline.vertices.size();
        const double bulge = index < polyline.bulges.size() ? polyline.bulges[index] : 0.0;
        const double candidate_distance = pointToPolylineSegmentDistance(
            pick_point, polyline.vertices[index], polyline.vertices[end_index], bulge);
        if (candidate_distance < best_distance)
        {
            best_distance = candidate_distance;
            picked_segment = index;
        }
    }
    VpEntityRecord segment = target_entity;
    const std::size_t segment_end = (picked_segment + 1) % polyline.vertices.size();
    const VpPoint2d segment_start_point = polyline.vertices[picked_segment];
    const VpPoint2d segment_end_point = polyline.vertices[segment_end];
    const double source_bulge =
        picked_segment < polyline.bulges.size() ? polyline.bulges[picked_segment] : 0.0;
    VpArcEntity segment_arc;
    if (bulgeArc(segment_start_point, segment_end_point, source_bulge, segment_arc))
    {
        segment.type = VpEntityType::Arc;
        segment.geometry = segment_arc;
    }
    else
    {
        segment.type = VpEntityType::Line;
        segment.geometry = VpLineEntity{segment_start_point, segment_end_point};
    }
    std::vector<VpEntityRecord> kept_segments =
        trimmedEntityParts(cutting_entity, segment, pick_point);
    if (kept_segments.empty())
    {
        return {};
    }
    const auto parameter = [&](const VpPoint2d& point)
    {
        return std::clamp(
            polylineSegmentParameter(segment_start_point, segment_end_point, source_bulge, point),
            0.0, 1.0);
    };
    std::vector<std::pair<double, double>> kept_intervals;
    for (const VpEntityRecord& kept_segment : kept_segments)
    {
        VpPoint2d kept_start;
        VpPoint2d kept_end;
        if (kept_segment.type == VpEntityType::Line)
        {
            const auto& line = std::get<VpLineEntity>(kept_segment.geometry);
            kept_start = line.start_point;
            kept_end = line.end_point;
        }
        else
        {
            const auto& arc = std::get<VpArcEntity>(kept_segment.geometry);
            const auto point_at_angle = [&](double angle)
            {
                const double radians = angle * kPi / 180.0;
                return VpPoint2d{arc.center.x + (arc.radius * std::cos(radians)),
                                 arc.center.y + (arc.radius * std::sin(radians))};
            };
            kept_start = point_at_angle(arc.start_angle);
            kept_end = point_at_angle(arc.end_angle);
        }
        double start_parameter = parameter(kept_start);
        double end_parameter = parameter(kept_end);
        if (start_parameter > end_parameter)
        {
            std::swap(start_parameter, end_parameter);
        }
        kept_intervals.push_back({start_parameter, end_parameter});
    }
    std::sort(kept_intervals.begin(), kept_intervals.end());
    double removed_start = 0.0;
    double removed_end = 1.0;
    if (kept_intervals.size() >= 2)
    {
        removed_start = kept_intervals.front().second;
        removed_end = kept_intervals.back().first;
    }
    else if (kept_intervals.front().first <= 1.0e-9)
    {
        removed_start = kept_intervals.front().second;
    }
    else
    {
        removed_end = kept_intervals.front().first;
    }
    const auto point_at = [&](double value)
    {
        return polylineSegmentPoint(segment_start_point, segment_end_point, source_bulge, value);
    };
    const auto add_source_edge =
        [&](std::size_t index, std::vector<VpPoint2d>& vertices, std::vector<double>& bulges)
    {
        const std::size_t end_index = (index + 1) % polyline.vertices.size();
        const double bulge = index < polyline.bulges.size() ? polyline.bulges[index] : 0.0;
        appendPolylineEdge(vertices, bulges, polyline.vertices[index], polyline.vertices[end_index],
                           bulge);
    };
    const auto add_trimmed_edge = [&](double start_parameter, double end_parameter,
                                      std::vector<VpPoint2d>& vertices, std::vector<double>& bulges)
    {
        appendPolylineEdge(vertices, bulges, point_at(start_parameter), point_at(end_parameter),
                           partialBulge(source_bulge, start_parameter, end_parameter));
    };
    std::vector<VpEntityRecord> results;
    if (polyline.is_closed)
    {
        std::vector<VpPoint2d> remaining_vertices;
        std::vector<double> remaining_bulges;
        add_trimmed_edge(removed_end, 1.0, remaining_vertices, remaining_bulges);
        std::size_t vertex_index = segment_end;
        while (vertex_index != picked_segment)
        {
            add_source_edge(vertex_index, remaining_vertices, remaining_bulges);
            vertex_index = (vertex_index + 1) % polyline.vertices.size();
        }
        add_trimmed_edge(0.0, removed_start, remaining_vertices, remaining_bulges);
        appendOpenPolylinePart(target_entity, std::move(remaining_vertices),
                               std::move(remaining_bulges), results);
        return results;
    }
    std::vector<VpPoint2d> left_vertices;
    std::vector<double> left_bulges;
    for (std::size_t index = 0; index < picked_segment; ++index)
    {
        add_source_edge(index, left_vertices, left_bulges);
    }
    add_trimmed_edge(0.0, removed_start, left_vertices, left_bulges);
    appendOpenPolylinePart(target_entity, std::move(left_vertices), std::move(left_bulges),
                           results);
    std::vector<VpPoint2d> right_vertices;
    std::vector<double> right_bulges;
    add_trimmed_edge(removed_end, 1.0, right_vertices, right_bulges);
    for (std::size_t index = picked_segment + 1; index < segment_count; ++index)
    {
        add_source_edge(index, right_vertices, right_bulges);
    }
    appendOpenPolylinePart(target_entity, std::move(right_vertices), std::move(right_bulges),
                           results);
    return results;
}

} // namespace

std::vector<VpEntityRecord> trimmedEntityParts(const VpEntityRecord& cutting_entity,
                                               const VpEntityRecord& target_entity,
                                               const VpPoint2d& pick_point)
{
    if (cutting_entity.id == target_entity.id)
    {
        return {};
    }
    if (target_entity.type == VpEntityType::Polyline)
    {
        return trimOpenPolyline(cutting_entity, target_entity, pick_point);
    }
    if (cutting_entity.type == VpEntityType::Line && target_entity.type == VpEntityType::Line)
    {
        VpEntityRecord result;
        return trimmedLineEntity(cutting_entity, target_entity, pick_point, result)
                   ? std::vector<VpEntityRecord>{result}
                   : std::vector<VpEntityRecord>{};
    }
    const std::vector<VpPoint2d> intersections = curveIntersections(cutting_entity, target_entity);
    if (intersections.empty())
    {
        return {};
    }
    if (target_entity.type == VpEntityType::Line)
    {
        return trimLine(target_entity, intersections, pick_point);
    }
    if (target_entity.type == VpEntityType::Circle)
    {
        if (intersections.size() < 2)
        {
            return {};
        }
        const auto& circle = std::get<VpCircleEntity>(target_entity.geometry);
        const double first_angle = entityAngleDegrees(circle.center, intersections[0]);
        const double second_angle = entityAngleDegrees(circle.center, intersections[1]);
        const double pick_angle = entityAngleDegrees(circle.center, pick_point);
        const bool pick_on_first_arc = counterClockwiseSpan(first_angle, pick_angle) <=
                                       counterClockwiseSpan(first_angle, second_angle);
        VpEntityRecord result = target_entity;
        result.type = VpEntityType::Arc;
        result.geometry =
            pick_on_first_arc
                ? VpArcEntity{circle.center, circle.radius, second_angle, first_angle}
                : VpArcEntity{circle.center, circle.radius, first_angle, second_angle};
        return {result};
    }
    const VpPoint2d center = target_entity.type == VpEntityType::Circle
                                 ? std::get<VpCircleEntity>(target_entity.geometry).center
                                 : std::get<VpArcEntity>(target_entity.geometry).center;
    const double radius = target_entity.type == VpEntityType::Circle
                              ? std::get<VpCircleEntity>(target_entity.geometry).radius
                              : std::get<VpArcEntity>(target_entity.geometry).radius;
    double start_angle = target_entity.type == VpEntityType::Circle
                             ? 0.0
                             : std::get<VpArcEntity>(target_entity.geometry).start_angle;
    const double total_span =
        target_entity.type == VpEntityType::Circle
            ? 360.0
            : counterClockwiseSpan(start_angle,
                                   std::get<VpArcEntity>(target_entity.geometry).end_angle);
    std::vector<double> boundaries{0.0, total_span};
    for (const VpPoint2d& point : intersections)
    {
        const double parameter =
            counterClockwiseSpan(start_angle, entityAngleDegrees(center, point));
        if (parameter > 1.0e-8 && parameter < total_span - 1.0e-8)
        {
            boundaries.push_back(parameter);
        }
    }
    std::sort(boundaries.begin(), boundaries.end());
    if (boundaries.size() <= 2)
    {
        return {};
    }
    const double pick_parameter =
        counterClockwiseSpan(start_angle, entityAngleDegrees(center, pick_point));
    std::size_t removed_interval = 0;
    while (removed_interval + 1 < boundaries.size() &&
           pick_parameter > boundaries[removed_interval + 1])
    {
        ++removed_interval;
    }
    std::vector<VpEntityRecord> results;
    for (std::size_t index = 0; index + 1 < boundaries.size(); ++index)
    {
        if (index == removed_interval)
        {
            continue;
        }
        VpEntityRecord result = target_entity;
        result.type = VpEntityType::Arc;
        result.geometry =
            VpArcEntity{center, radius, normalizedAngle(start_angle + boundaries[index]),
                        normalizedAngle(start_angle + boundaries[index + 1])};
        results.push_back(std::move(result));
    }
    return results;
}

void VpCadViewport::drawTrimPreview(QPainter& painter)
{
    const std::optional<VpEntityId> hovered_id = entityAt(m_cursor_world);
    const VpEntityRecord* hovered = hovered_id ? entityById(*hovered_id) : nullptr;
    if (!m_reference_entity_id)
    {
        if (hovered &&
            (hovered->type == VpEntityType::Line || hovered->type == VpEntityType::Circle ||
             hovered->type == VpEntityType::Arc))
        {
            painter.setPen(QPen(QColor(105, 219, 142), 2.0, Qt::DashLine));
            drawEntityGeometry(painter, *hovered, QColor(105, 219, 142, 60));
        }
        return;
    }
    const VpEntityRecord* cutting = entityById(*m_reference_entity_id);
    const std::vector<VpEntityRecord> previews =
        cutting && hovered ? trimmedEntityParts(*cutting, *hovered, m_cursor_world)
                           : std::vector<VpEntityRecord>{};
    painter.setPen(QPen(QColor(92, 214, 255), 1.8, Qt::DashLine));
    for (const VpEntityRecord& preview : previews)
    {
        drawEntityGeometry(painter, preview, QColor(92, 214, 255, 60));
    }
}

} // namespace Vp
