#include "vp_toolpath.h"

#include "vp_ellipse_geometry.h"
#include "vp_spline_geometry.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Vp
{
namespace
{

constexpr double kDistanceEpsilon = 1.0e-12;
constexpr double kPi = 3.14159265358979323846;

VpPoint2d polarPoint(const VpPoint2d& center, double radius, double angle_degrees)
{
    const double radians = angle_degrees * kPi / 180.0;
    return {center.x + (std::cos(radians) * radius), center.y + (std::sin(radians) * radius)};
}

void appendPolylineSegment(std::vector<VpPoint2d>& points, const VpPoint2d& start,
                           const VpPoint2d& end, double bulge)
{
    if (points.empty())
    {
        points.push_back(start);
    }
    const double chord = distance(start, end);
    if (std::abs(bulge) <= 1.0e-12 || chord <= 1.0e-12)
    {
        points.push_back(end);
        return;
    }
    const double sweep = 4.0 * std::atan(bulge);
    const double center_offset = chord / (2.0 * std::tan(sweep * 0.5));
    const VpPoint2d midpoint{(start.x + end.x) * 0.5, (start.y + end.y) * 0.5};
    const VpPoint2d center{midpoint.x - ((end.y - start.y) * center_offset / chord),
                           midpoint.y + ((end.x - start.x) * center_offset / chord)};
    const double radius = distance(center, start);
    const double start_angle = std::atan2(start.y - center.y, start.x - center.x) * 180.0 / kPi;
    const int segments =
        std::max(2, static_cast<int>(std::ceil(std::abs(sweep) * 180.0 / kPi / 8.0)));
    for (int index = 1; index <= segments; ++index)
    {
        const double ratio = static_cast<double>(index) / segments;
        points.push_back(polarPoint(center, radius, start_angle + (sweep * 180.0 / kPi * ratio)));
    }
}

std::vector<VpPoint2d> entityPathPoints(const VpEntityRecord& entity)
{
    if (entity.type == VpEntityType::Line)
    {
        const auto& line = std::get<VpLineEntity>(entity.geometry);
        return {line.start_point, line.end_point};
    }
    if (entity.type == VpEntityType::Arc)
    {
        const auto& arc = std::get<VpArcEntity>(entity.geometry);
        double sweep =
            arc.is_clockwise ? arc.start_angle - arc.end_angle : arc.end_angle - arc.start_angle;
        while (sweep <= 0.0)
        {
            sweep += 360.0;
        }
        const int segments = std::max(2, static_cast<int>(std::ceil(sweep / 8.0)));
        std::vector<VpPoint2d> points;
        points.reserve(static_cast<std::size_t>(segments + 1));
        for (int index = 0; index <= segments; ++index)
        {
            const double ratio = static_cast<double>(index) / segments;
            const double angle = arc.start_angle + (arc.is_clockwise ? -sweep : sweep) * ratio;
            points.push_back(polarPoint(arc.center, arc.radius, angle));
        }
        return points;
    }
    if (entity.type == VpEntityType::Polyline)
    {
        const auto& polyline = std::get<VpPolylineEntity>(entity.geometry);
        std::vector<VpPoint2d> points;
        if (polyline.vertices.size() < 2)
        {
            return points;
        }
        const std::size_t segment_count =
            polyline.is_closed ? polyline.vertices.size() : polyline.vertices.size() - 1;
        for (std::size_t index = 0; index < segment_count; ++index)
        {
            const std::size_t end_index = (index + 1) % polyline.vertices.size();
            const double bulge = index < polyline.bulges.size() ? polyline.bulges[index] : 0.0;
            appendPolylineSegment(points, polyline.vertices[index], polyline.vertices[end_index],
                                  bulge);
        }
        return points;
    }
    if (entity.type == VpEntityType::Spline)
    {
        return splineApproximation(std::get<VpSplineEntity>(entity.geometry));
    }
    if (entity.type == VpEntityType::Ellipse)
    {
        std::vector<VpPoint2d> points =
            ellipseApproximation(std::get<VpEllipseEntity>(entity.geometry));
        if (!points.empty() && distance(points.front(), points.back()) > kDistanceEpsilon)
        {
            points.push_back(points.front());
        }
        return points;
    }
    if (entity.type == VpEntityType::Circle)
    {
        const auto& circle = std::get<VpCircleEntity>(entity.geometry);
        std::vector<VpPoint2d> points;
        constexpr int kSegments = 72;
        points.reserve(kSegments + 1);
        for (int index = 0; index <= kSegments; ++index)
        {
            points.push_back(polarPoint(circle.center, circle.radius,
                                        360.0 * static_cast<double>(index) / kSegments));
        }
        return points;
    }
    return {};
}

double rowTolerance(const std::vector<VpEntityRecord>& entities)
{
    double minimum_x = std::numeric_limits<double>::max();
    double minimum_y = std::numeric_limits<double>::max();
    double maximum_x = std::numeric_limits<double>::lowest();
    double maximum_y = std::numeric_limits<double>::lowest();
    for (const VpEntityRecord& entity : entities)
    {
        const VpPoint2d point = toolpathStartPoint(entity);
        minimum_x = std::min(minimum_x, point.x);
        minimum_y = std::min(minimum_y, point.y);
        maximum_x = std::max(maximum_x, point.x);
        maximum_y = std::max(maximum_y, point.y);
    }
    return std::max(1.0e-6, std::hypot(maximum_x - minimum_x, maximum_y - minimum_y) * 0.001);
}

VpToolpathSortResult rowScanSort(const std::vector<VpEntityRecord>& entities,
                                 const VpToolpathSortOptions& options)
{
    VpToolpathSortResult result;
    result.entities = entities;
    const bool top_first = options.vertical == VpToolpathVerticalDirection::TopToBottom;
    std::stable_sort(result.entities.begin(), result.entities.end(),
                     [top_first](const VpEntityRecord& first, const VpEntityRecord& second)
                     {
                         const double first_y = toolpathStartPoint(first).y;
                         const double second_y = toolpathStartPoint(second).y;
                         return top_first ? first_y > second_y : first_y < second_y;
                     });
    const double tolerance = rowTolerance(result.entities);
    std::size_t row_begin = 0;
    while (row_begin < result.entities.size())
    {
        const double baseline_y = toolpathStartPoint(result.entities[row_begin]).y;
        std::size_t row_end = row_begin + 1;
        while (row_end < result.entities.size() &&
               std::abs(toolpathStartPoint(result.entities[row_end]).y - baseline_y) <= tolerance)
        {
            ++row_end;
        }
        const bool left_first = options.horizontal == VpToolpathHorizontalDirection::LeftToRight;
        std::stable_sort(result.entities.begin() + static_cast<std::ptrdiff_t>(row_begin),
                         result.entities.begin() + static_cast<std::ptrdiff_t>(row_end),
                         [left_first](const VpEntityRecord& first, const VpEntityRecord& second)
                         {
                             const double first_x = toolpathStartPoint(first).x;
                             const double second_x = toolpathStartPoint(second).x;
                             return left_first ? first_x < second_x : first_x > second_x;
                         });
        row_begin = row_end;
    }
    return result;
}

VpToolpathSortResult shortestSort(const std::vector<VpEntityRecord>& entities,
                                  const VpToolpathSortOptions& options)
{
    VpToolpathSortResult result;
    std::vector<VpEntityRecord> remaining = entities;
    VpPoint2d current = options.shortest_start;
    while (!remaining.empty())
    {
        std::size_t best_index = 0;
        bool reverse_best = false;
        double best_distance = std::numeric_limits<double>::max();
        for (std::size_t index = 0; index < remaining.size(); ++index)
        {
            const double start_distance = distance(current, toolpathStartPoint(remaining[index]));
            double candidate_distance = start_distance;
            bool reverse_candidate = false;
            if (options.allow_reverse && canReverseToolpath(remaining[index]))
            {
                const double end_distance = distance(current, toolpathEndPoint(remaining[index]));
                if (end_distance + kDistanceEpsilon < start_distance)
                {
                    candidate_distance = end_distance;
                    reverse_candidate = true;
                }
            }
            if (candidate_distance + kDistanceEpsilon < best_distance)
            {
                best_index = index;
                best_distance = candidate_distance;
                reverse_best = reverse_candidate;
            }
        }
        VpEntityRecord selected = remaining[best_index];
        if (reverse_best)
        {
            selected = reversedToolpathEntity(selected);
            result.reversed_entity_ids.push_back(selected.id);
        }
        current = toolpathEndPoint(selected);
        result.entities.push_back(std::move(selected));
        remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(best_index));
    }
    return result;
}

} // namespace

bool isMachinableEntity(const VpEntityRecord& entity) noexcept
{
    return entity.type == VpEntityType::Line || entity.type == VpEntityType::Circle ||
           entity.type == VpEntityType::Arc || entity.type == VpEntityType::Polyline ||
           entity.type == VpEntityType::Spline || entity.type == VpEntityType::Ellipse;
}

VpPoint2d toolpathStartPoint(const VpEntityRecord& entity) noexcept
{
    if (entity.type == VpEntityType::Line)
    {
        return std::get<VpLineEntity>(entity.geometry).start_point;
    }
    if (entity.type == VpEntityType::Arc)
    {
        const auto& arc = std::get<VpArcEntity>(entity.geometry);
        return polarPoint(arc.center, arc.radius, arc.start_angle);
    }
    if (entity.type == VpEntityType::Polyline)
    {
        const auto& polyline = std::get<VpPolylineEntity>(entity.geometry);
        return polyline.vertices.empty() ? VpPoint2d{} : polyline.vertices.front();
    }
    if (entity.type == VpEntityType::Spline)
    {
        return std::get<VpSplineEntity>(entity.geometry).control_points.front();
    }
    if (entity.type == VpEntityType::Circle)
    {
        const auto& circle = std::get<VpCircleEntity>(entity.geometry);
        return {circle.center.x + circle.radius, circle.center.y};
    }
    if (entity.type == VpEntityType::Ellipse)
    {
        const auto& ellipse = std::get<VpEllipseEntity>(entity.geometry);
        return {ellipse.center.x + ellipse.major_axis.x, ellipse.center.y + ellipse.major_axis.y};
    }
    return {};
}

VpPoint2d toolpathEndPoint(const VpEntityRecord& entity) noexcept
{
    if (entity.type == VpEntityType::Line)
    {
        return std::get<VpLineEntity>(entity.geometry).end_point;
    }
    if (entity.type == VpEntityType::Arc)
    {
        const auto& arc = std::get<VpArcEntity>(entity.geometry);
        return polarPoint(arc.center, arc.radius, arc.end_angle);
    }
    if (entity.type == VpEntityType::Polyline)
    {
        const auto& polyline = std::get<VpPolylineEntity>(entity.geometry);
        if (polyline.vertices.empty() || polyline.is_closed)
        {
            return toolpathStartPoint(entity);
        }
        return polyline.vertices.back();
    }
    if (entity.type == VpEntityType::Spline)
    {
        return std::get<VpSplineEntity>(entity.geometry).control_points.back();
    }
    return toolpathStartPoint(entity);
}

bool canReverseToolpath(const VpEntityRecord& entity) noexcept
{
    if (entity.type == VpEntityType::Polyline)
    {
        return !std::get<VpPolylineEntity>(entity.geometry).is_closed;
    }
    return entity.type == VpEntityType::Line || entity.type == VpEntityType::Arc ||
           entity.type == VpEntityType::Spline;
}

VpEntityRecord reversedToolpathEntity(const VpEntityRecord& entity)
{
    VpEntityRecord result = entity;
    if (result.type == VpEntityType::Line)
    {
        auto& line = std::get<VpLineEntity>(result.geometry);
        std::swap(line.start_point, line.end_point);
    }
    else if (result.type == VpEntityType::Arc)
    {
        auto& arc = std::get<VpArcEntity>(result.geometry);
        std::swap(arc.start_angle, arc.end_angle);
        arc.is_clockwise = !arc.is_clockwise;
    }
    else if (result.type == VpEntityType::Spline)
    {
        auto& controls = std::get<VpSplineEntity>(result.geometry).control_points;
        std::reverse(controls.begin(), controls.end());
    }
    else if (result.type == VpEntityType::Polyline)
    {
        auto& polyline = std::get<VpPolylineEntity>(result.geometry);
        if (!polyline.is_closed)
        {
            const std::size_t count = polyline.vertices.size();
            const std::vector<double> old_bulges = polyline.bulges;
            const std::vector<double> old_start_widths = polyline.start_widths;
            const std::vector<double> old_end_widths = polyline.end_widths;
            std::reverse(polyline.vertices.begin(), polyline.vertices.end());
            polyline.bulges.assign(count, 0.0);
            polyline.start_widths.assign(count, 0.0);
            polyline.end_widths.assign(count, 0.0);
            for (std::size_t index = 0; index + 1 < count; ++index)
            {
                const std::size_t source_index = count - 2 - index;
                if (source_index < old_bulges.size())
                {
                    polyline.bulges[index] = -old_bulges[source_index];
                }
                if (source_index < old_end_widths.size())
                {
                    polyline.start_widths[index] = old_end_widths[source_index];
                }
                if (source_index < old_start_widths.size())
                {
                    polyline.end_widths[index] = old_start_widths[source_index];
                }
            }
        }
    }
    return result;
}

VpToolpathSortResult sortToolpathEntities(const std::vector<VpEntityRecord>& entities,
                                          const VpToolpathSortOptions& options)
{
    if (entities.empty())
    {
        return {};
    }
    return options.mode == VpToolpathSortMode::Shortest ? shortestSort(entities, options)
                                                        : rowScanSort(entities, options);
}

std::vector<VpToolpathMotion> generateToolpathMotions(const std::vector<VpEntityRecord>& entities,
                                                      const VpPoint2d& initial_position)
{
    std::vector<VpToolpathMotion> motions;
    VpPoint2d current = initial_position;
    for (const VpEntityRecord& entity : entities)
    {
        if (!isMachinableEntity(entity))
        {
            continue;
        }
        const std::vector<VpPoint2d> points = entityPathPoints(entity);
        if (points.size() < 2)
        {
            continue;
        }
        motions.push_back({VpToolpathMotionType::Rapid, current, points.front(), entity.id});
        for (std::size_t index = 1; index < points.size(); ++index)
        {
            motions.push_back(
                {VpToolpathMotionType::Cutting, points[index - 1], points[index], entity.id});
        }
        current = points.back();
    }
    return motions;
}

} // namespace Vp
