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

std::vector<VpPoint2d> lineArcIntersections(const VpLineEntity& line, const VpArcEntity& arc)
{
    std::vector<VpPoint2d> intersections;
    const double direction_x = line.end_point.x - line.start_point.x;
    const double direction_y = line.end_point.y - line.start_point.y;
    const double length_squared = (direction_x * direction_x) + (direction_y * direction_y);
    if (length_squared <= 1.0e-18)
    {
        return intersections;
    }
    const double offset_x = line.start_point.x - arc.center.x;
    const double offset_y = line.start_point.y - arc.center.y;
    const double half_linear = (offset_x * direction_x) + (offset_y * direction_y);
    const double constant =
        (offset_x * offset_x) + (offset_y * offset_y) - (arc.radius * arc.radius);
    const double discriminant = (half_linear * half_linear) - (length_squared * constant);
    if (discriminant < -1.0e-9)
    {
        return intersections;
    }
    const double root = std::sqrt(std::max(0.0, discriminant));
    for (double parameter :
         {(-half_linear - root) / length_squared, (-half_linear + root) / length_squared})
    {
        const VpPoint2d point{line.start_point.x + (parameter * direction_x),
                              line.start_point.y + (parameter * direction_y)};
        if (angleOnArc(entityAngleDegrees(arc.center, point), arc) &&
            (intersections.empty() || distance(intersections.front(), point) > 1.0e-8))
        {
            intersections.push_back(point);
        }
    }
    return intersections;
}

std::vector<VpPoint2d> arcArcIntersections(const VpArcEntity& first_arc,
                                           const VpArcEntity& second_arc)
{
    std::vector<VpPoint2d> intersections;
    const double center_distance = distance(first_arc.center, second_arc.center);
    if (center_distance <= 1.0e-9 ||
        center_distance > first_arc.radius + second_arc.radius + 1.0e-9 ||
        center_distance < std::abs(first_arc.radius - second_arc.radius) - 1.0e-9)
    {
        return intersections;
    }
    const double along =
        ((first_arc.radius * first_arc.radius) - (second_arc.radius * second_arc.radius) +
         (center_distance * center_distance)) /
        (2.0 * center_distance);
    const double height_squared = (first_arc.radius * first_arc.radius) - (along * along);
    if (height_squared < -1.0e-9)
    {
        return intersections;
    }
    const double unit_x = (second_arc.center.x - first_arc.center.x) / center_distance;
    const double unit_y = (second_arc.center.y - first_arc.center.y) / center_distance;
    const VpPoint2d base{first_arc.center.x + (along * unit_x),
                         first_arc.center.y + (along * unit_y)};
    const double height = std::sqrt(std::max(0.0, height_squared));
    for (const VpPoint2d& point :
         {VpPoint2d{base.x - (height * unit_y), base.y + (height * unit_x)},
          VpPoint2d{base.x + (height * unit_y), base.y - (height * unit_x)}})
    {
        if (angleOnArc(entityAngleDegrees(first_arc.center, point), first_arc) &&
            angleOnArc(entityAngleDegrees(second_arc.center, point), second_arc) &&
            (intersections.empty() || distance(intersections.front(), point) > 1.0e-8))
        {
            intersections.push_back(point);
        }
    }
    return intersections;
}

bool trimLine(const VpEntityRecord& source, const VpPoint2d& intersection,
              const VpPoint2d& pick_point, double trim_distance, VpEntityRecord& result,
              VpPoint2d& trim_point)
{
    const auto& line = std::get<VpLineEntity>(source.geometry);
    const double line_x = line.end_point.x - line.start_point.x;
    const double line_y = line.end_point.y - line.start_point.y;
    const double line_length = std::hypot(line_x, line_y);
    if (line_length <= 1.0e-9)
    {
        return false;
    }
    VpPoint2d direction{line_x / line_length, line_y / line_length};
    double pick_projection = ((pick_point.x - intersection.x) * direction.x) +
                             ((pick_point.y - intersection.y) * direction.y);
    if (std::abs(pick_projection) <= 1.0e-9)
    {
        const double start_projection = ((line.start_point.x - intersection.x) * direction.x) +
                                        ((line.start_point.y - intersection.y) * direction.y);
        const double end_projection = ((line.end_point.x - intersection.x) * direction.x) +
                                      ((line.end_point.y - intersection.y) * direction.y);
        pick_projection = std::abs(start_projection) > std::abs(end_projection) ? start_projection
                                                                                : end_projection;
    }
    if (pick_projection < 0.0)
    {
        direction = {-direction.x, -direction.y};
    }
    const double start_projection = ((line.start_point.x - intersection.x) * direction.x) +
                                    ((line.start_point.y - intersection.y) * direction.y);
    const double end_projection = ((line.end_point.x - intersection.x) * direction.x) +
                                  ((line.end_point.y - intersection.y) * direction.y);
    const VpPoint2d retained =
        start_projection >= end_projection ? line.start_point : line.end_point;
    trim_point = {intersection.x + (direction.x * trim_distance),
                  intersection.y + (direction.y * trim_distance)};
    if (distance(retained, trim_point) <= 1.0e-9)
    {
        return false;
    }
    result = source;
    result.geometry = VpLineEntity{retained, trim_point};
    return true;
}

bool trimArc(const VpEntityRecord& source, const VpPoint2d& intersection,
             const VpPoint2d& pick_point, double trim_distance, VpEntityRecord& result,
             VpPoint2d& trim_point)
{
    const auto& arc = std::get<VpArcEntity>(source.geometry);
    const double span = counterClockwiseSpan(arc.start_angle, arc.end_angle);
    const double intersection_angle = entityAngleDegrees(arc.center, intersection);
    const double intersection_position = counterClockwiseSpan(arc.start_angle, intersection_angle);
    const double pick_position =
        counterClockwiseSpan(arc.start_angle, entityAngleDegrees(arc.center, pick_point));
    if (intersection_position > span + 1.0e-8 || pick_position > span + 1.0e-8)
    {
        return false;
    }
    const bool retain_start = pick_position < intersection_position;
    const double available_angle =
        retain_start ? intersection_position : span - intersection_position;
    const double trim_angle_delta = trim_distance / arc.radius * 180.0 / kPi;
    if (trim_angle_delta >= available_angle - 1.0e-9)
    {
        return false;
    }
    const double trim_angle =
        normalizedAngle(intersection_angle + (retain_start ? -trim_angle_delta : trim_angle_delta));
    trim_point = arcPoint(arc, trim_angle);
    result = source;
    auto& result_arc = std::get<VpArcEntity>(result.geometry);
    if (retain_start)
    {
        result_arc.end_angle = trim_angle;
    }
    else
    {
        result_arc.start_angle = trim_angle;
    }
    return true;
}

bool trimEntity(const VpEntityRecord& source, const VpPoint2d& intersection,
                const VpPoint2d& pick_point, double trim_distance, VpEntityRecord& result,
                VpPoint2d& trim_point)
{
    if (source.type == VpEntityType::Line)
    {
        return trimLine(source, intersection, pick_point, trim_distance, result, trim_point);
    }
    if (source.type == VpEntityType::Arc)
    {
        return trimArc(source, intersection, pick_point, trim_distance, result, trim_point);
    }
    return false;
}

} // namespace

bool chamferedEntities(const VpEntityRecord& first_source, const VpEntityRecord& second_source,
                       const VpPoint2d& first_pick, const VpPoint2d& second_pick,
                       double first_distance, double second_distance, VpEntityRecord& first_result,
                       VpEntityRecord& second_result, VpEntityRecord& chamfer_result)
{
    if (first_source.id == second_source.id || first_distance <= 1.0e-9 ||
        second_distance <= 1.0e-9)
    {
        return false;
    }
    if (first_source.type == VpEntityType::Line && second_source.type == VpEntityType::Line)
    {
        return chamferedLineEntities(first_source, second_source, first_pick, second_pick,
                                     first_distance, second_distance, first_result, second_result,
                                     chamfer_result);
    }
    std::vector<VpPoint2d> intersections;
    if (first_source.type == VpEntityType::Line && second_source.type == VpEntityType::Arc)
    {
        intersections = lineArcIntersections(std::get<VpLineEntity>(first_source.geometry),
                                             std::get<VpArcEntity>(second_source.geometry));
    }
    else if (first_source.type == VpEntityType::Arc && second_source.type == VpEntityType::Line)
    {
        intersections = lineArcIntersections(std::get<VpLineEntity>(second_source.geometry),
                                             std::get<VpArcEntity>(first_source.geometry));
    }
    else if (first_source.type == VpEntityType::Arc && second_source.type == VpEntityType::Arc)
    {
        intersections = arcArcIntersections(std::get<VpArcEntity>(first_source.geometry),
                                            std::get<VpArcEntity>(second_source.geometry));
    }
    if (intersections.empty())
    {
        return false;
    }
    const VpPoint2d& intersection =
        *std::min_element(intersections.begin(), intersections.end(),
                          [&](const VpPoint2d& first, const VpPoint2d& second)
                          {
                              return distance(first, first_pick) + distance(first, second_pick) <
                                     distance(second, first_pick) + distance(second, second_pick);
                          });
    VpPoint2d first_trim;
    VpPoint2d second_trim;
    if (!trimEntity(first_source, intersection, first_pick, first_distance, first_result,
                    first_trim) ||
        !trimEntity(second_source, intersection, second_pick, second_distance, second_result,
                    second_trim) ||
        distance(first_trim, second_trim) <= 1.0e-9)
    {
        return false;
    }
    chamfer_result = first_source;
    chamfer_result.type = VpEntityType::Line;
    chamfer_result.geometry = VpLineEntity{first_trim, second_trim};
    return true;
}

} // namespace Vp
