#include "s_cad_viewport_geometry.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <vector>

namespace vectorPath
{
namespace
{

double normalizedAngle(double angle)
{
    double result = std::fmod(angle, 360.0);
    return result < 0.0 ? result + 360.0 : result;
}

double counterClockwiseSpan(double start_angle, double end_angle)
{
    return normalizedAngle(end_angle - start_angle);
}

SPoint2d arcPoint(const SArcEntity& arc, double angle)
{
    const double radians = angle * 3.14159265358979323846 / 180.0;
    return {arc.center.x + (arc.radius * std::cos(radians)),
            arc.center.y + (arc.radius * std::sin(radians))};
}

bool angleOnArc(double angle, const SArcEntity& arc)
{
    return counterClockwiseSpan(arc.start_angle, angle) <=
           counterClockwiseSpan(arc.start_angle, arc.end_angle) + 1.0e-8;
}

std::vector<std::pair<SPoint2d, double>>
infiniteLineCircleIntersections(const SLineEntity& line, const SPoint2d& center, double radius)
{
    std::vector<std::pair<SPoint2d, double>> results;
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
        return results;
    }
    const double root = std::sqrt(std::max(0.0, discriminant));
    for (double parameter : {(-coefficient_b - root) / (2.0 * coefficient_a),
                             (-coefficient_b + root) / (2.0 * coefficient_a)})
    {
        results.push_back({{line.start_point.x + (parameter * delta_x),
                            line.start_point.y + (parameter * delta_y)},
                           parameter});
    }
    return results;
}

std::vector<SPoint2d> circleCircleIntersections(const SPoint2d& first_center, double first_radius,
                                                const SPoint2d& second_center, double second_radius)
{
    std::vector<SPoint2d> points;
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
    const SPoint2d base{first_center.x + (along * unit_x), first_center.y + (along * unit_y)};
    const double height = std::sqrt(std::max(0.0, height_squared));
    points.push_back({base.x - (height * unit_y), base.y + (height * unit_x)});
    if (height > 1.0e-8)
    {
        points.push_back({base.x + (height * unit_y), base.y - (height * unit_x)});
    }
    return points;
}

bool boundaryAcceptsPoint(const SEntityRecord& boundary, const SPoint2d& point)
{
    if (boundary.type != SEntityType::Arc)
    {
        return true;
    }
    const auto& arc = std::get<SArcEntity>(boundary.geometry);
    return angleOnArc(entityAngleDegrees(arc.center, point), arc);
}

std::vector<SPoint2d> arcBoundaryIntersections(const SArcEntity& target_arc,
                                               const SEntityRecord& boundary)
{
    std::vector<SPoint2d> points;
    if (boundary.type == SEntityType::Line)
    {
        for (const auto& intersection : infiniteLineCircleIntersections(
                 std::get<SLineEntity>(boundary.geometry), target_arc.center, target_arc.radius))
        {
            if (intersection.second >= -1.0e-9 && intersection.second <= 1.0 + 1.0e-9)
            {
                points.push_back(intersection.first);
            }
        }
    }
    else if (boundary.type == SEntityType::Circle || boundary.type == SEntityType::Arc)
    {
        const SPoint2d center = boundary.type == SEntityType::Circle
                                    ? std::get<SCircleEntity>(boundary.geometry).center
                                    : std::get<SArcEntity>(boundary.geometry).center;
        const double radius = boundary.type == SEntityType::Circle
                                  ? std::get<SCircleEntity>(boundary.geometry).radius
                                  : std::get<SArcEntity>(boundary.geometry).radius;
        points = circleCircleIntersections(target_arc.center, target_arc.radius, center, radius);
    }
    points.erase(std::remove_if(points.begin(), points.end(),
                                [&](const SPoint2d& point)
                                {
                                    return !boundaryAcceptsPoint(boundary, point);
                                }),
                 points.end());
    return points;
}

} // namespace

bool extendedEntity(const SEntityRecord& boundary_entity, const SEntityRecord& target_entity,
                    const SPoint2d& pick_point, SEntityRecord& result)
{
    if (boundary_entity.id == target_entity.id ||
        (boundary_entity.type != SEntityType::Line && boundary_entity.type != SEntityType::Circle &&
         boundary_entity.type != SEntityType::Arc))
    {
        return false;
    }
    if (target_entity.type == SEntityType::Line)
    {
        const auto& target_line = std::get<SLineEntity>(target_entity.geometry);
        std::vector<std::pair<SPoint2d, double>> candidates;
        if (boundary_entity.type == SEntityType::Line)
        {
            const auto& boundary_line = std::get<SLineEntity>(boundary_entity.geometry);
            const double boundary_x = boundary_line.end_point.x - boundary_line.start_point.x;
            const double boundary_y = boundary_line.end_point.y - boundary_line.start_point.y;
            const double target_x = target_line.end_point.x - target_line.start_point.x;
            const double target_y = target_line.end_point.y - target_line.start_point.y;
            const double denominator = (boundary_x * target_y) - (boundary_y * target_x);
            if (std::abs(denominator) <= 1.0e-12)
            {
                return false;
            }
            const double origin_x = target_line.start_point.x - boundary_line.start_point.x;
            const double origin_y = target_line.start_point.y - boundary_line.start_point.y;
            const double boundary_parameter =
                ((origin_x * target_y) - (origin_y * target_x)) / denominator;
            const double target_parameter =
                ((origin_x * boundary_y) - (origin_y * boundary_x)) / denominator;
            if (boundary_parameter < -1.0e-9 || boundary_parameter > 1.0 + 1.0e-9)
            {
                return false;
            }
            candidates.push_back({{target_line.start_point.x + (target_parameter * target_x),
                                   target_line.start_point.y + (target_parameter * target_y)},
                                  target_parameter});
        }
        else
        {
            const SPoint2d center = boundary_entity.type == SEntityType::Circle
                                        ? std::get<SCircleEntity>(boundary_entity.geometry).center
                                        : std::get<SArcEntity>(boundary_entity.geometry).center;
            const double radius = boundary_entity.type == SEntityType::Circle
                                      ? std::get<SCircleEntity>(boundary_entity.geometry).radius
                                      : std::get<SArcEntity>(boundary_entity.geometry).radius;
            candidates = infiniteLineCircleIntersections(target_line, center, radius);
            candidates.erase(std::remove_if(candidates.begin(), candidates.end(),
                                            [&](const auto& candidate)
                                            {
                                                return !boundaryAcceptsPoint(boundary_entity,
                                                                             candidate.first);
                                            }),
                             candidates.end());
        }
        const bool extend_start = distance(pick_point, target_line.start_point) <=
                                  distance(pick_point, target_line.end_point);
        double best_distance = std::numeric_limits<double>::max();
        std::optional<SPoint2d> best_point;
        for (const auto& candidate : candidates)
        {
            const double extension = extend_start ? -candidate.second : candidate.second - 1.0;
            if (extension > 1.0e-9 && extension < best_distance)
            {
                best_distance = extension;
                best_point = candidate.first;
            }
        }
        if (!best_point)
        {
            return false;
        }
        result = target_entity;
        auto& line = std::get<SLineEntity>(result.geometry);
        (extend_start ? line.start_point : line.end_point) = *best_point;
        return true;
    }
    if (target_entity.type == SEntityType::Polyline)
    {
        const auto& polyline = std::get<SPolylineEntity>(target_entity.geometry);
        if (polyline.is_closed || polyline.vertices.size() < 2)
        {
            return false;
        }
        const bool extend_start = distance(pick_point, polyline.vertices.front()) <=
                                  distance(pick_point, polyline.vertices.back());
        const std::size_t segment_index = extend_start ? 0 : polyline.vertices.size() - 2;
        const double segment_bulge =
            segment_index < polyline.bulges.size() ? polyline.bulges[segment_index] : 0.0;
        SEntityRecord endpoint_segment = target_entity;
        SArcEntity source_arc;
        if (bulgeArc(polyline.vertices[segment_index], polyline.vertices[segment_index + 1],
                     segment_bulge, source_arc))
        {
            endpoint_segment.type = SEntityType::Arc;
            endpoint_segment.geometry = source_arc;
        }
        else
        {
            endpoint_segment.type = SEntityType::Line;
            endpoint_segment.geometry =
                SLineEntity{polyline.vertices[segment_index], polyline.vertices[segment_index + 1]};
        }
        SEntityRecord extended_segment;
        if (!extendedEntity(boundary_entity, endpoint_segment, pick_point, extended_segment))
        {
            return false;
        }
        result = target_entity;
        auto& result_polyline = std::get<SPolylineEntity>(result.geometry);
        if (extended_segment.type == SEntityType::Line)
        {
            const auto& result_line = std::get<SLineEntity>(extended_segment.geometry);
            (extend_start ? result_polyline.vertices.front() : result_polyline.vertices.back()) =
                extend_start ? result_line.start_point : result_line.end_point;
        }
        else
        {
            const auto& result_arc = std::get<SArcEntity>(extended_segment.geometry);
            const SPoint2d arc_start = arcPoint(result_arc, result_arc.start_angle);
            const SPoint2d arc_end = arcPoint(result_arc, result_arc.end_angle);
            if (extend_start)
            {
                result_polyline.vertices.front() = segment_bulge > 0.0 ? arc_start : arc_end;
            }
            else
            {
                result_polyline.vertices.back() = segment_bulge > 0.0 ? arc_end : arc_start;
            }
            result_polyline.bulges.resize(result_polyline.vertices.size(), 0.0);
            const double new_bulge =
                std::tan(counterClockwiseSpan(result_arc.start_angle, result_arc.end_angle) *
                         3.14159265358979323846 / 720.0);
            result_polyline.bulges[segment_index] = std::copysign(new_bulge, segment_bulge);
        }
        return true;
    }
    if (target_entity.type != SEntityType::Arc)
    {
        return false;
    }
    const auto& target_arc = std::get<SArcEntity>(target_entity.geometry);
    const std::vector<SPoint2d> candidates = arcBoundaryIntersections(target_arc, boundary_entity);
    const SPoint2d start_point{
        target_arc.center.x +
            (target_arc.radius * std::cos(target_arc.start_angle * 3.14159265358979323846 / 180.0)),
        target_arc.center.y + (target_arc.radius *
                               std::sin(target_arc.start_angle * 3.14159265358979323846 / 180.0))};
    const SPoint2d end_point{
        target_arc.center.x +
            (target_arc.radius * std::cos(target_arc.end_angle * 3.14159265358979323846 / 180.0)),
        target_arc.center.y +
            (target_arc.radius * std::sin(target_arc.end_angle * 3.14159265358979323846 / 180.0))};
    const bool extend_start = distance(pick_point, start_point) <= distance(pick_point, end_point);
    double best_extension = std::numeric_limits<double>::max();
    double best_angle = 0.0;
    for (const SPoint2d& candidate : candidates)
    {
        const double angle = entityAngleDegrees(target_arc.center, candidate);
        if (angleOnArc(angle, target_arc))
        {
            continue;
        }
        const double extension = extend_start ? counterClockwiseSpan(angle, target_arc.start_angle)
                                              : counterClockwiseSpan(target_arc.end_angle, angle);
        if (extension > 1.0e-8 && extension < best_extension)
        {
            best_extension = extension;
            best_angle = angle;
        }
    }
    if (!std::isfinite(best_extension))
    {
        return false;
    }
    result = target_entity;
    auto& arc = std::get<SArcEntity>(result.geometry);
    (extend_start ? arc.start_angle : arc.end_angle) = best_angle;
    return true;
}

} // namespace vectorPath
