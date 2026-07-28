#include "s_curve_offset.h"

#include "s_cad_viewport_geometry.h"
#include "s_spline_geometry.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace smartGraphics
{
namespace
{

constexpr double kPi = 3.14159265358979323846;
constexpr double kGeometryEpsilon = 1.0e-9;

double crossProduct(const SPoint2d& first_vector, const SPoint2d& second_vector) noexcept
{
    return (first_vector.x * second_vector.y) - (first_vector.y * second_vector.x);
}

SPoint2d subtractPoints(const SPoint2d& first_point, const SPoint2d& second_point) noexcept
{
    return {first_point.x - second_point.x, first_point.y - second_point.y};
}

SPoint2d interpolatePoint(const SPoint2d& first_point, const SPoint2d& second_point,
                          double parameter) noexcept
{
    return {first_point.x + ((second_point.x - first_point.x) * parameter),
            first_point.y + ((second_point.y - first_point.y) * parameter)};
}

double pointSegmentDistance(const SPoint2d& point, const SPoint2d& start_point,
                            const SPoint2d& end_point, SPoint2d* closest_point = nullptr) noexcept
{
    const SPoint2d direction = subtractPoints(end_point, start_point);
    const double length_squared = (direction.x * direction.x) + (direction.y * direction.y);
    const double parameter = length_squared <= kGeometryEpsilon * kGeometryEpsilon
                                 ? 0.0
                                 : std::clamp(((point.x - start_point.x) * direction.x +
                                               (point.y - start_point.y) * direction.y) /
                                                  length_squared,
                                              0.0, 1.0);
    const SPoint2d projection = interpolatePoint(start_point, end_point, parameter);
    if (closest_point)
    {
        *closest_point = projection;
    }
    return distance(point, projection);
}

void appendUniquePoint(std::vector<SPoint2d>& points, const SPoint2d& point)
{
    if (points.empty() || distance(points.back(), point) > kGeometryEpsilon)
    {
        points.push_back(point);
    }
}

std::vector<SPoint2d> flattenedPolyline(const SPolylineEntity& polyline)
{
    std::vector<SPoint2d> points;
    if (polyline.vertices.size() < 2)
    {
        return points;
    }
    const std::size_t segment_count =
        polyline.is_closed ? polyline.vertices.size() : polyline.vertices.size() - 1;
    appendUniquePoint(points, polyline.vertices.front());
    for (std::size_t segment_index = 0; segment_index < segment_count; ++segment_index)
    {
        const SPoint2d& start_point = polyline.vertices[segment_index];
        const SPoint2d& end_point =
            polyline.vertices[(segment_index + 1) % polyline.vertices.size()];
        const double bulge =
            segment_index < polyline.bulges.size() ? polyline.bulges[segment_index] : 0.0;
        SArcEntity arc;
        if (!bulgeArc(start_point, end_point, bulge, arc))
        {
            appendUniquePoint(points, end_point);
            continue;
        }
        const double signed_span = 4.0 * std::atan(bulge);
        const int sample_count =
            std::clamp(static_cast<int>(std::ceil(std::abs(signed_span) / (kPi / 36.0))), 2, 720);
        const double start_angle =
            std::atan2(start_point.y - arc.center.y, start_point.x - arc.center.x);
        for (int sample_index = 1; sample_index <= sample_count; ++sample_index)
        {
            const double parameter = static_cast<double>(sample_index) / sample_count;
            const double angle = start_angle + (signed_span * parameter);
            appendUniquePoint(points, {arc.center.x + (arc.radius * std::cos(angle)),
                                       arc.center.y + (arc.radius * std::sin(angle))});
        }
    }
    if (polyline.is_closed && points.size() > 2 &&
        distance(points.front(), points.back()) <= kGeometryEpsilon)
    {
        points.pop_back();
    }
    return points;
}

void appendAdaptiveSpline(const SSplineEntity& spline, double first_parameter,
                          double second_parameter, const SPoint2d& first_point,
                          const SPoint2d& second_point, double tolerance, int depth,
                          std::vector<SPoint2d>& points)
{
    const double middle_parameter = (first_parameter + second_parameter) * 0.5;
    const double first_quarter_parameter = (first_parameter + middle_parameter) * 0.5;
    const double third_quarter_parameter = (middle_parameter + second_parameter) * 0.5;
    const SPoint2d middle_point = splinePoint(spline, middle_parameter);
    const SPoint2d first_quarter_point = splinePoint(spline, first_quarter_parameter);
    const SPoint2d third_quarter_point = splinePoint(spline, third_quarter_parameter);
    const double deviation =
        std::max({pointSegmentDistance(middle_point, first_point, second_point),
                  pointSegmentDistance(first_quarter_point, first_point, second_point),
                  pointSegmentDistance(third_quarter_point, first_point, second_point)});
    if (depth >= 14 || deviation <= tolerance)
    {
        appendUniquePoint(points, second_point);
        return;
    }
    appendAdaptiveSpline(spline, first_parameter, middle_parameter, first_point, middle_point,
                         tolerance, depth + 1, points);
    appendAdaptiveSpline(spline, middle_parameter, second_parameter, middle_point, second_point,
                         tolerance, depth + 1, points);
}

std::vector<SPoint2d> flattenedSpline(const SSplineEntity& spline)
{
    double minimum_x = spline.control_points.front().x;
    double minimum_y = spline.control_points.front().y;
    double maximum_x = minimum_x;
    double maximum_y = minimum_y;
    for (const SPoint2d& point : spline.control_points)
    {
        minimum_x = std::min(minimum_x, point.x);
        minimum_y = std::min(minimum_y, point.y);
        maximum_x = std::max(maximum_x, point.x);
        maximum_y = std::max(maximum_y, point.y);
    }
    const double diagonal = std::hypot(maximum_x - minimum_x, maximum_y - minimum_y);
    const double tolerance = std::max(diagonal * 1.0e-4, 1.0e-6);
    std::vector<SPoint2d> points{spline.control_points.front()};
    appendAdaptiveSpline(spline, 0.0, 1.0, spline.control_points.front(),
                         spline.control_points.back(), tolerance, 0, points);
    return points;
}

bool lineIntersection(const SPoint2d& first_origin, const SPoint2d& first_direction,
                      const SPoint2d& second_origin, const SPoint2d& second_direction,
                      SPoint2d& intersection) noexcept
{
    const double denominator = crossProduct(first_direction, second_direction);
    if (std::abs(denominator) <= 1.0e-12)
    {
        return false;
    }
    const double parameter =
        crossProduct(subtractPoints(second_origin, first_origin), second_direction) / denominator;
    intersection = {first_origin.x + (first_direction.x * parameter),
                    first_origin.y + (first_direction.y * parameter)};
    return true;
}

bool segmentIntersection(const SPoint2d& first_start, const SPoint2d& first_end,
                         const SPoint2d& second_start, const SPoint2d& second_end,
                         SPoint2d& intersection) noexcept
{
    const SPoint2d first_direction = subtractPoints(first_end, first_start);
    const SPoint2d second_direction = subtractPoints(second_end, second_start);
    const double denominator = crossProduct(first_direction, second_direction);
    if (std::abs(denominator) <= 1.0e-12)
    {
        return false;
    }
    const SPoint2d origin_delta = subtractPoints(second_start, first_start);
    const double first_parameter = crossProduct(origin_delta, second_direction) / denominator;
    const double second_parameter = crossProduct(origin_delta, first_direction) / denominator;
    if (first_parameter <= 1.0e-8 || first_parameter >= 1.0 - 1.0e-8 ||
        second_parameter <= 1.0e-8 || second_parameter >= 1.0 - 1.0e-8)
    {
        return false;
    }
    intersection = {first_start.x + (first_direction.x * first_parameter),
                    first_start.y + (first_direction.y * first_parameter)};
    return true;
}

double signedArea(const std::vector<SPoint2d>& points) noexcept
{
    if (points.size() < 3)
    {
        return 0.0;
    }
    double area = 0.0;
    for (std::size_t index = 0; index < points.size(); ++index)
    {
        const SPoint2d& first_point = points[index];
        const SPoint2d& second_point = points[(index + 1) % points.size()];
        area += (first_point.x * second_point.y) - (second_point.x * first_point.y);
    }
    return area * 0.5;
}

void removeOpenPathLoops(std::vector<SPoint2d>& points)
{
    for (int pass = 0; pass < 16; ++pass)
    {
        bool did_remove = false;
        for (std::size_t first_index = 0; first_index + 2 < points.size() && !did_remove;
             ++first_index)
        {
            for (std::size_t second_index = first_index + 2; second_index + 1 < points.size();
                 ++second_index)
            {
                SPoint2d intersection;
                if (!segmentIntersection(points[first_index], points[first_index + 1],
                                         points[second_index], points[second_index + 1],
                                         intersection))
                {
                    continue;
                }
                points.erase(points.begin() + static_cast<std::ptrdiff_t>(first_index + 1),
                             points.begin() + static_cast<std::ptrdiff_t>(second_index + 1));
                points.insert(points.begin() + static_cast<std::ptrdiff_t>(first_index + 1),
                              intersection);
                did_remove = true;
                break;
            }
        }
        if (!did_remove)
        {
            return;
        }
    }
}

void removeClosedPathLoops(std::vector<SPoint2d>& points, bool keep_larger_loop)
{
    for (int pass = 0; pass < 16; ++pass)
    {
        bool did_remove = false;
        const std::size_t point_count = points.size();
        for (std::size_t first_index = 0; first_index < point_count && !did_remove; ++first_index)
        {
            const std::size_t first_end_index = (first_index + 1) % point_count;
            for (std::size_t second_index = first_index + 2; second_index < point_count;
                 ++second_index)
            {
                const std::size_t second_end_index = (second_index + 1) % point_count;
                if (second_end_index == first_index)
                {
                    continue;
                }
                SPoint2d intersection;
                if (!segmentIntersection(points[first_index], points[first_end_index],
                                         points[second_index], points[second_end_index],
                                         intersection))
                {
                    continue;
                }
                std::vector<SPoint2d> first_loop{intersection};
                for (std::size_t index = first_end_index; index <= second_index; ++index)
                {
                    first_loop.push_back(points[index]);
                }
                std::vector<SPoint2d> second_loop{intersection};
                for (std::size_t index = second_end_index; index != first_end_index;
                     index = (index + 1) % point_count)
                {
                    second_loop.push_back(points[index]);
                }
                const bool first_is_larger =
                    std::abs(signedArea(first_loop)) >= std::abs(signedArea(second_loop));
                points = (first_is_larger == keep_larger_loop) ? std::move(first_loop)
                                                               : std::move(second_loop);
                did_remove = true;
                break;
            }
        }
        if (!did_remove || points.size() < 3)
        {
            return;
        }
    }
}

bool selectedOffset(const std::vector<SPoint2d>& points, bool is_closed,
                    const SPoint2d& through_point, double& offset_distance) noexcept
{
    const std::size_t segment_count = is_closed ? points.size() : points.size() - 1;
    double nearest_distance = std::numeric_limits<double>::max();
    double nearest_side = 0.0;
    for (std::size_t index = 0; index < segment_count; ++index)
    {
        const SPoint2d& start_point = points[index];
        const SPoint2d& end_point = points[(index + 1) % points.size()];
        SPoint2d closest_point;
        const double candidate_distance =
            pointSegmentDistance(through_point, start_point, end_point, &closest_point);
        if (candidate_distance >= nearest_distance)
        {
            continue;
        }
        const SPoint2d direction = subtractPoints(end_point, start_point);
        const double direction_length = std::hypot(direction.x, direction.y);
        if (direction_length <= kGeometryEpsilon)
        {
            continue;
        }
        nearest_distance = candidate_distance;
        nearest_side = crossProduct(direction, subtractPoints(through_point, closest_point)) /
                       direction_length;
    }
    if (!std::isfinite(nearest_distance) || nearest_distance <= kGeometryEpsilon ||
        std::abs(nearest_side) <= kGeometryEpsilon)
    {
        return false;
    }
    offset_distance = std::copysign(nearest_distance, nearest_side);
    return true;
}

bool offsetPath(const std::vector<SPoint2d>& source_points, bool is_closed,
                const SPoint2d& through_point, std::vector<SPoint2d>& result_points)
{
    std::vector<SPoint2d> points;
    points.reserve(source_points.size());
    for (const SPoint2d& point : source_points)
    {
        appendUniquePoint(points, point);
    }
    if (points.size() < (is_closed ? 3U : 2U))
    {
        return false;
    }
    double offset_distance = 0.0;
    if (!selectedOffset(points, is_closed, through_point, offset_distance))
    {
        return false;
    }
    const std::size_t edge_count = is_closed ? points.size() : points.size() - 1;
    std::vector<SPoint2d> directions(edge_count);
    std::vector<SPoint2d> normals(edge_count);
    for (std::size_t edge_index = 0; edge_index < edge_count; ++edge_index)
    {
        const SPoint2d direction =
            subtractPoints(points[(edge_index + 1) % points.size()], points[edge_index]);
        const double edge_length = std::hypot(direction.x, direction.y);
        if (edge_length <= kGeometryEpsilon)
        {
            return false;
        }
        directions[edge_index] = {direction.x / edge_length, direction.y / edge_length};
        normals[edge_index] = {-directions[edge_index].y, directions[edge_index].x};
    }
    result_points.clear();
    result_points.reserve(points.size() + 8);
    for (std::size_t point_index = 0; point_index < points.size(); ++point_index)
    {
        if (!is_closed && point_index == 0)
        {
            appendUniquePoint(result_points,
                              {points.front().x + (normals.front().x * offset_distance),
                               points.front().y + (normals.front().y * offset_distance)});
            continue;
        }
        if (!is_closed && point_index + 1 == points.size())
        {
            appendUniquePoint(result_points,
                              {points.back().x + (normals.back().x * offset_distance),
                               points.back().y + (normals.back().y * offset_distance)});
            continue;
        }
        const std::size_t previous_edge = (point_index + edge_count - 1) % edge_count;
        const std::size_t next_edge = point_index % edge_count;
        const SPoint2d previous_origin{
            points[point_index].x + (normals[previous_edge].x * offset_distance),
            points[point_index].y + (normals[previous_edge].y * offset_distance)};
        const SPoint2d next_origin{points[point_index].x + (normals[next_edge].x * offset_distance),
                                   points[point_index].y +
                                       (normals[next_edge].y * offset_distance)};
        SPoint2d intersection;
        const bool has_intersection =
            lineIntersection(previous_origin, directions[previous_edge], next_origin,
                             directions[next_edge], intersection);
        const double maximum_miter = std::abs(offset_distance) * 12.0 + kGeometryEpsilon;
        if (has_intersection && distance(intersection, points[point_index]) <= maximum_miter)
        {
            appendUniquePoint(result_points, intersection);
        }
        else
        {
            appendUniquePoint(result_points, previous_origin);
            appendUniquePoint(result_points, next_origin);
        }
    }
    if (is_closed)
    {
        const bool is_outward = signedArea(points) * offset_distance < 0.0;
        removeClosedPathLoops(result_points, is_outward);
    }
    else
    {
        removeOpenPathLoops(result_points);
    }
    return result_points.size() >= (is_closed ? 3U : 2U);
}

} // namespace

bool offsetCurveEntity(const SEntityRecord& source, const SPoint2d& through_point,
                       SEntityRecord& result)
{
    std::vector<SPoint2d> source_points;
    bool is_closed = false;
    if (source.type == SEntityType::Polyline)
    {
        const auto& polyline = std::get<SPolylineEntity>(source.geometry);
        source_points = flattenedPolyline(polyline);
        is_closed = polyline.is_closed;
    }
    else if (source.type == SEntityType::Spline)
    {
        source_points = flattenedSpline(std::get<SSplineEntity>(source.geometry));
    }
    else
    {
        return false;
    }
    std::vector<SPoint2d> offset_points;
    if (!offsetPath(source_points, is_closed, through_point, offset_points))
    {
        return false;
    }
    result = source;
    result.type = SEntityType::Polyline;
    result.geometry = SPolylineEntity{std::move(offset_points), is_closed};
    result.associative_array.reset();
    return true;
}

} // namespace smartGraphics
