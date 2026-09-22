#include "vp_cad_viewport_geometry.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace Vp
{
namespace
{

constexpr double kPi = 3.14159265358979323846;

double normalizedAngle(double angle)
{
    const double result = std::fmod(angle, 360.0);
    return result < 0.0 ? result + 360.0 : result;
}

double counterClockwiseSpan(double start_angle, double end_angle)
{
    return normalizedAngle(end_angle - start_angle);
}

bool angleOnArc(double angle, const VpArcEntity& arc)
{
    return counterClockwiseSpan(arc.start_angle, angle) <=
           counterClockwiseSpan(arc.start_angle, arc.end_angle) + 1.0e-8;
}

VpPoint2d arcPoint(const VpArcEntity& arc, double angle)
{
    const double radians = angle * kPi / 180.0;
    return {arc.center.x + (arc.radius * std::cos(radians)),
            arc.center.y + (arc.radius * std::sin(radians))};
}

struct VpFilletCandidate
{
    VpPoint2d center;
    VpPoint2d line_tangent;
    VpPoint2d curve_tangent;
    double score = std::numeric_limits<double>::max();
};

void appendCandidateCenters(const VpPoint2d& line_origin, const VpPoint2d& line_direction,
                            const VpPoint2d& curve_center, double center_distance,
                            std::vector<VpPoint2d>& centers)
{
    const double origin_x = line_origin.x - curve_center.x;
    const double origin_y = line_origin.y - curve_center.y;
    const double projection = -((origin_x * line_direction.x) + (origin_y * line_direction.y));
    const double closest_x = origin_x + (projection * line_direction.x);
    const double closest_y = origin_y + (projection * line_direction.y);
    const double height_squared =
        (center_distance * center_distance) - ((closest_x * closest_x) + (closest_y * closest_y));
    if (height_squared < -1.0e-9)
    {
        return;
    }
    const double offset = std::sqrt(std::max(0.0, height_squared));
    for (double parameter : {projection - offset, projection + offset})
    {
        const VpPoint2d center{line_origin.x + (parameter * line_direction.x),
                               line_origin.y + (parameter * line_direction.y)};
        if (centers.empty() || std::none_of(centers.begin(), centers.end(),
                                            [&](const VpPoint2d& existing)
                                            {
                                                return distance(existing, center) <= 1.0e-8;
                                            }))
        {
            centers.push_back(center);
        }
    }
}

std::vector<VpPoint2d> circleIntersections(const VpPoint2d& first_center, double first_radius,
                                           const VpPoint2d& second_center, double second_radius)
{
    std::vector<VpPoint2d> points;
    const double center_distance = distance(first_center, second_center);
    if (center_distance <= 1.0e-9 || center_distance > first_radius + second_radius + 1.0e-9 ||
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

bool curveGeometry(const VpEntityRecord& source, VpPoint2d& center, double& radius)
{
    if (source.type == VpEntityType::Circle)
    {
        const auto& circle = std::get<VpCircleEntity>(source.geometry);
        center = circle.center;
        radius = circle.radius;
        return true;
    }
    if (source.type == VpEntityType::Arc)
    {
        const auto& arc = std::get<VpArcEntity>(source.geometry);
        center = arc.center;
        radius = arc.radius;
        return true;
    }
    return false;
}

void trimArcResult(const VpEntityRecord& source, const VpPoint2d& pick_point,
                   const VpPoint2d& tangent_point, VpEntityRecord& result)
{
    result = source;
    if (source.type != VpEntityType::Arc)
    {
        return;
    }
    const auto& source_arc = std::get<VpArcEntity>(source.geometry);
    auto& result_arc = std::get<VpArcEntity>(result.geometry);
    const VpPoint2d start_point = arcPoint(source_arc, source_arc.start_angle);
    const VpPoint2d end_point = arcPoint(source_arc, source_arc.end_angle);
    const double tangent_angle = entityAngleDegrees(source_arc.center, tangent_point);
    if (distance(pick_point, start_point) <= distance(pick_point, end_point))
    {
        result_arc.end_angle = tangent_angle;
    }
    else
    {
        result_arc.start_angle = tangent_angle;
    }
}

bool curveCurveFillet(const VpEntityRecord& first_source, const VpEntityRecord& second_source,
                      const VpPoint2d& first_pick, const VpPoint2d& second_pick, double radius,
                      VpEntityRecord& first_result, VpEntityRecord& second_result,
                      VpEntityRecord& fillet_result)
{
    VpPoint2d first_center;
    VpPoint2d second_center;
    double first_radius = 0.0;
    double second_radius = 0.0;
    if (!curveGeometry(first_source, first_center, first_radius) ||
        !curveGeometry(second_source, second_center, second_radius))
    {
        return false;
    }
    std::vector<double> first_offsets{first_radius + radius};
    std::vector<double> second_offsets{second_radius + radius};
    if (first_radius > radius + 1.0e-9)
    {
        first_offsets.push_back(first_radius - radius);
    }
    if (second_radius > radius + 1.0e-9)
    {
        second_offsets.push_back(second_radius - radius);
    }
    std::vector<VpFilletCandidate> candidates;
    for (double first_offset : first_offsets)
    {
        for (double second_offset : second_offsets)
        {
            for (const VpPoint2d& center :
                 circleIntersections(first_center, first_offset, second_center, second_offset))
            {
                const double first_distance = distance(first_center, center);
                const double second_distance = distance(second_center, center);
                const VpPoint2d first_tangent{
                    first_center.x + ((center.x - first_center.x) * first_radius / first_distance),
                    first_center.y + ((center.y - first_center.y) * first_radius / first_distance)};
                const VpPoint2d second_tangent{second_center.x + ((center.x - second_center.x) *
                                                                  second_radius / second_distance),
                                               second_center.y + ((center.y - second_center.y) *
                                                                  second_radius / second_distance)};
                if ((first_source.type == VpEntityType::Arc &&
                     !angleOnArc(entityAngleDegrees(first_center, first_tangent),
                                 std::get<VpArcEntity>(first_source.geometry))) ||
                    (second_source.type == VpEntityType::Arc &&
                     !angleOnArc(entityAngleDegrees(second_center, second_tangent),
                                 std::get<VpArcEntity>(second_source.geometry))))
                {
                    continue;
                }
                candidates.push_back(
                    {center, first_tangent, second_tangent,
                     distance(first_pick, first_tangent) + distance(second_pick, second_tangent)});
            }
        }
    }
    if (candidates.empty())
    {
        return false;
    }
    const VpFilletCandidate& best =
        *std::min_element(candidates.begin(), candidates.end(),
                          [](const VpFilletCandidate& first, const VpFilletCandidate& second)
                          {
                              return first.score < second.score;
                          });
    trimArcResult(first_source, first_pick, best.line_tangent, first_result);
    trimArcResult(second_source, second_pick, best.curve_tangent, second_result);
    double start_angle = entityAngleDegrees(best.center, best.line_tangent);
    double end_angle = entityAngleDegrees(best.center, best.curve_tangent);
    if (counterClockwiseSpan(start_angle, end_angle) > 180.0)
    {
        std::swap(start_angle, end_angle);
    }
    fillet_result = first_source;
    fillet_result.type = VpEntityType::Arc;
    fillet_result.geometry = VpArcEntity{best.center, radius, start_angle, end_angle};
    return true;
}

bool curveFillet(const VpEntityRecord& line_source, const VpEntityRecord& curve_source,
                 const VpPoint2d& line_pick, const VpPoint2d& curve_pick, double radius,
                 VpEntityRecord& line_result, VpEntityRecord& curve_result,
                 VpEntityRecord& fillet_result)
{
    const auto& line = std::get<VpLineEntity>(line_source.geometry);
    const double line_x = line.end_point.x - line.start_point.x;
    const double line_y = line.end_point.y - line.start_point.y;
    const double line_length = std::hypot(line_x, line_y);
    if (line_length <= 1.0e-9)
    {
        return false;
    }
    const VpPoint2d direction{line_x / line_length, line_y / line_length};
    const VpPoint2d normal{-direction.y, direction.x};
    const VpPoint2d curve_center = curve_source.type == VpEntityType::Circle
                                       ? std::get<VpCircleEntity>(curve_source.geometry).center
                                       : std::get<VpArcEntity>(curve_source.geometry).center;
    const double curve_radius = curve_source.type == VpEntityType::Circle
                                    ? std::get<VpCircleEntity>(curve_source.geometry).radius
                                    : std::get<VpArcEntity>(curve_source.geometry).radius;
    std::vector<VpFilletCandidate> candidates;
    for (double side : {-1.0, 1.0})
    {
        const VpPoint2d offset_origin{line.start_point.x + (side * radius * normal.x),
                                      line.start_point.y + (side * radius * normal.y)};
        std::vector<VpPoint2d> centers;
        appendCandidateCenters(offset_origin, direction, curve_center, curve_radius + radius,
                               centers);
        if (curve_radius > radius + 1.0e-9)
        {
            appendCandidateCenters(offset_origin, direction, curve_center, curve_radius - radius,
                                   centers);
        }
        for (const VpPoint2d& center : centers)
        {
            const double center_distance = distance(center, curve_center);
            if (center_distance <= 1.0e-9)
            {
                continue;
            }
            const double projection = ((center.x - line.start_point.x) * direction.x) +
                                      ((center.y - line.start_point.y) * direction.y);
            const VpPoint2d line_tangent{line.start_point.x + (projection * direction.x),
                                         line.start_point.y + (projection * direction.y)};
            const VpPoint2d curve_tangent{
                curve_center.x + ((center.x - curve_center.x) * curve_radius / center_distance),
                curve_center.y + ((center.y - curve_center.y) * curve_radius / center_distance)};
            if (curve_source.type == VpEntityType::Arc &&
                !angleOnArc(entityAngleDegrees(curve_center, curve_tangent),
                            std::get<VpArcEntity>(curve_source.geometry)))
            {
                continue;
            }
            candidates.push_back(
                {center, line_tangent, curve_tangent,
                 distance(line_pick, line_tangent) + distance(curve_pick, curve_tangent)});
        }
    }
    if (candidates.empty())
    {
        return false;
    }
    const VpFilletCandidate& best =
        *std::min_element(candidates.begin(), candidates.end(),
                          [](const VpFilletCandidate& first, const VpFilletCandidate& second)
                          {
                              return first.score < second.score;
                          });
    const VpPoint2d retained_line_point =
        distance(line_pick, line.start_point) <= distance(line_pick, line.end_point)
            ? line.start_point
            : line.end_point;
    if (distance(retained_line_point, best.line_tangent) <= 1.0e-9)
    {
        return false;
    }
    line_result = line_source;
    line_result.geometry = VpLineEntity{retained_line_point, best.line_tangent};
    curve_result = curve_source;
    if (curve_source.type == VpEntityType::Arc)
    {
        const auto& source_arc = std::get<VpArcEntity>(curve_source.geometry);
        auto& result_arc = std::get<VpArcEntity>(curve_result.geometry);
        const VpPoint2d start_point = arcPoint(source_arc, source_arc.start_angle);
        const VpPoint2d end_point = arcPoint(source_arc, source_arc.end_angle);
        const double tangent_angle = entityAngleDegrees(source_arc.center, best.curve_tangent);
        if (distance(curve_pick, start_point) <= distance(curve_pick, end_point))
        {
            result_arc.end_angle = tangent_angle;
        }
        else
        {
            result_arc.start_angle = tangent_angle;
        }
    }
    double start_angle = entityAngleDegrees(best.center, best.line_tangent);
    double end_angle = entityAngleDegrees(best.center, best.curve_tangent);
    if (counterClockwiseSpan(start_angle, end_angle) > 180.0)
    {
        std::swap(start_angle, end_angle);
    }
    fillet_result = line_source;
    fillet_result.type = VpEntityType::Arc;
    fillet_result.geometry = VpArcEntity{best.center, radius, start_angle, end_angle};
    return true;
}

} // namespace

bool filletedEntities(const VpEntityRecord& first_source, const VpEntityRecord& second_source,
                      const VpPoint2d& first_pick, const VpPoint2d& second_pick, double radius,
                      VpEntityRecord& first_result, VpEntityRecord& second_result,
                      VpEntityRecord& arc_result)
{
    if (first_source.id == second_source.id || radius <= 1.0e-9)
    {
        return false;
    }
    if (first_source.type == VpEntityType::Line && second_source.type == VpEntityType::Line)
    {
        return filletedLineEntities(first_source, second_source, first_pick, second_pick, radius,
                                    first_result, second_result, arc_result);
    }
    const bool first_is_curve =
        first_source.type == VpEntityType::Circle || first_source.type == VpEntityType::Arc;
    const bool second_is_curve =
        second_source.type == VpEntityType::Circle || second_source.type == VpEntityType::Arc;
    if (first_source.type == VpEntityType::Line && second_is_curve)
    {
        return curveFillet(first_source, second_source, first_pick, second_pick, radius,
                           first_result, second_result, arc_result);
    }
    if (first_is_curve && second_source.type == VpEntityType::Line)
    {
        return curveFillet(second_source, first_source, second_pick, first_pick, radius,
                           second_result, first_result, arc_result);
    }
    if (first_is_curve && second_is_curve)
    {
        return curveCurveFillet(first_source, second_source, first_pick, second_pick, radius,
                                first_result, second_result, arc_result);
    }
    return false;
}

} // namespace Vp
