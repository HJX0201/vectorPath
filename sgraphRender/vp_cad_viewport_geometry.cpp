#include "vp_cad_viewport_geometry.h"

#include "vp_curve_offset.h"
#include "vp_dimension_geometry.h"

#include <algorithm>
#include <cmath>

namespace Vp
{
namespace
{

constexpr double kPi = 3.14159265358979323846;

bool lineIntersection(const VpLineEntity& first_line, const VpLineEntity& second_line,
                      double& first_parameter, double& second_parameter, VpPoint2d& intersection)
{
    const double first_delta_x = first_line.end_point.x - first_line.start_point.x;
    const double first_delta_y = first_line.end_point.y - first_line.start_point.y;
    const double second_delta_x = second_line.end_point.x - second_line.start_point.x;
    const double second_delta_y = second_line.end_point.y - second_line.start_point.y;
    const double denominator = (first_delta_x * second_delta_y) - (first_delta_y * second_delta_x);
    if (std::abs(denominator) <= 1.0e-12)
    {
        return false;
    }
    const double origin_delta_x = second_line.start_point.x - first_line.start_point.x;
    const double origin_delta_y = second_line.start_point.y - first_line.start_point.y;
    first_parameter =
        ((origin_delta_x * second_delta_y) - (origin_delta_y * second_delta_x)) / denominator;
    second_parameter =
        ((origin_delta_x * first_delta_y) - (origin_delta_y * first_delta_x)) / denominator;
    intersection = {first_line.start_point.x + (first_parameter * first_delta_x),
                    first_line.start_point.y + (first_parameter * first_delta_y)};
    return true;
}

bool lineSegmentIntersection(const VpLineEntity& first_line, const VpLineEntity& second_line,
                             VpPoint2d& intersection)
{
    double first_parameter = 0.0;
    double second_parameter = 0.0;
    if (!lineIntersection(first_line, second_line, first_parameter, second_parameter, intersection))
    {
        return false;
    }
    if (first_parameter < 0.0 || first_parameter > 1.0 || second_parameter < 0.0 ||
        second_parameter > 1.0)
    {
        return false;
    }
    return true;
}

} // namespace

bool calculateThreePointArc(const VpPoint2d& first_point, const VpPoint2d& second_point,
                            const VpPoint2d& third_point, VpPoint2d& center, double& radius,
                            double& start_angle, double& end_angle) noexcept
{
    const double denominator = 2.0 * ((first_point.x * (second_point.y - third_point.y)) +
                                      (second_point.x * (third_point.y - first_point.y)) +
                                      (third_point.x * (first_point.y - second_point.y)));
    if (std::abs(denominator) <= 1.0e-12)
    {
        return false;
    }

    const double first_squared = (first_point.x * first_point.x) + (first_point.y * first_point.y);
    const double second_squared =
        (second_point.x * second_point.x) + (second_point.y * second_point.y);
    const double third_squared = (third_point.x * third_point.x) + (third_point.y * third_point.y);
    center.x = ((first_squared * (second_point.y - third_point.y)) +
                (second_squared * (third_point.y - first_point.y)) +
                (third_squared * (first_point.y - second_point.y))) /
               denominator;
    center.y = ((first_squared * (third_point.x - second_point.x)) +
                (second_squared * (first_point.x - third_point.x)) +
                (third_squared * (second_point.x - first_point.x))) /
               denominator;
    radius = distance(center, first_point);
    if (radius <= 1.0e-9)
    {
        return false;
    }

    start_angle = entityAngleDegrees(center, first_point);
    const double middle_angle = entityAngleDegrees(center, second_point);
    end_angle = entityAngleDegrees(center, third_point);
    const double span_angle = std::fmod(end_angle - start_angle + 360.0, 360.0);
    const double middle_span = std::fmod(middle_angle - start_angle + 360.0, 360.0);
    if (middle_span > span_angle)
    {
        std::swap(start_angle, end_angle);
    }
    return true;
}

double entityAngleDegrees(const VpPoint2d& center, const VpPoint2d& point) noexcept
{
    double angle = std::atan2(point.y - center.y, point.x - center.x) * 180.0 / kPi;
    if (angle < 0.0)
    {
        angle += 360.0;
    }
    return angle;
}

bool offsetEntity(const VpEntityRecord& source, const VpPoint2d& through_point,
                  VpEntityRecord& result)
{
    result = source;
    if (source.type == VpEntityType::Line)
    {
        const auto& line = std::get<VpLineEntity>(source.geometry);
        const double delta_x = line.end_point.x - line.start_point.x;
        const double delta_y = line.end_point.y - line.start_point.y;
        const double line_length = std::hypot(delta_x, delta_y);
        if (line_length <= 1.0e-9)
        {
            return false;
        }
        const double normal_x = -delta_y / line_length;
        const double normal_y = delta_x / line_length;
        const double offset = ((through_point.x - line.start_point.x) * normal_x) +
                              ((through_point.y - line.start_point.y) * normal_y);
        result.geometry = VpLineEntity{
            {line.start_point.x + (normal_x * offset), line.start_point.y + (normal_y * offset)},
            {line.end_point.x + (normal_x * offset), line.end_point.y + (normal_y * offset)},
        };
        return true;
    }
    if (source.type == VpEntityType::Circle)
    {
        auto circle = std::get<VpCircleEntity>(source.geometry);
        circle.radius = distance(circle.center, through_point);
        result.geometry = circle;
        return circle.radius > 1.0e-9;
    }
    if (source.type == VpEntityType::Arc)
    {
        auto arc = std::get<VpArcEntity>(source.geometry);
        arc.radius = distance(arc.center, through_point);
        result.geometry = arc;
        return arc.radius > 1.0e-9;
    }
    if (source.type == VpEntityType::Polyline || source.type == VpEntityType::Spline)
    {
        return offsetCurveEntity(source, through_point, result);
    }
    return false;
}

bool trimmedLineEntity(const VpEntityRecord& cutting_entity, const VpEntityRecord& target_entity,
                       const VpPoint2d& pick_point, VpEntityRecord& result)
{
    if (cutting_entity.type != VpEntityType::Line || target_entity.type != VpEntityType::Line ||
        cutting_entity.id == target_entity.id)
    {
        return false;
    }
    const auto& cutting_line = std::get<VpLineEntity>(cutting_entity.geometry);
    const auto& target_line = std::get<VpLineEntity>(target_entity.geometry);
    VpPoint2d intersection;
    if (!lineSegmentIntersection(cutting_line, target_line, intersection))
    {
        return false;
    }
    result = target_entity;
    auto& replacement_line = std::get<VpLineEntity>(result.geometry);
    if (distance(pick_point, target_line.start_point) <=
        distance(pick_point, target_line.end_point))
    {
        replacement_line.start_point = intersection;
    }
    else
    {
        replacement_line.end_point = intersection;
    }
    return true;
}

bool extendedLineEntity(const VpEntityRecord& boundary_entity, const VpEntityRecord& target_entity,
                        VpEntityRecord& result)
{
    if (boundary_entity.type != VpEntityType::Line || target_entity.type != VpEntityType::Line ||
        boundary_entity.id == target_entity.id)
    {
        return false;
    }
    const auto& boundary_line = std::get<VpLineEntity>(boundary_entity.geometry);
    const auto& target_line = std::get<VpLineEntity>(target_entity.geometry);
    double boundary_parameter = 0.0;
    double target_parameter = 0.0;
    VpPoint2d intersection;
    if (!lineIntersection(boundary_line, target_line, boundary_parameter, target_parameter,
                          intersection) ||
        boundary_parameter < -1.0e-9 || boundary_parameter > 1.0 + 1.0e-9)
    {
        return false;
    }

    result = target_entity;
    auto& replacement_line = std::get<VpLineEntity>(result.geometry);
    if (target_parameter < -1.0e-9)
    {
        replacement_line.start_point = intersection;
        return true;
    }
    if (target_parameter > 1.0 + 1.0e-9)
    {
        replacement_line.end_point = intersection;
        return true;
    }
    return false;
}

std::vector<VpEntityRecord> brokenLineEntities(const VpEntityRecord& source,
                                               const VpPoint2d& first_break_point,
                                               const VpPoint2d& second_break_point)
{
    std::vector<VpEntityRecord> results;
    if (source.type != VpEntityType::Line)
    {
        return results;
    }
    const auto& line = std::get<VpLineEntity>(source.geometry);
    const double delta_x = line.end_point.x - line.start_point.x;
    const double delta_y = line.end_point.y - line.start_point.y;
    const double length_squared = (delta_x * delta_x) + (delta_y * delta_y);
    if (length_squared <= 1.0e-18)
    {
        return results;
    }
    const auto point_parameter = [&](const VpPoint2d& point)
    {
        return std::clamp((((point.x - line.start_point.x) * delta_x) +
                           ((point.y - line.start_point.y) * delta_y)) /
                              length_squared,
                          0.0, 1.0);
    };
    double first_parameter = point_parameter(first_break_point);
    double second_parameter = point_parameter(second_break_point);
    if (first_parameter > second_parameter)
    {
        std::swap(first_parameter, second_parameter);
    }
    if (std::abs(first_parameter - second_parameter) <= 1.0e-9 &&
        (first_parameter <= 1.0e-9 || first_parameter >= 1.0 - 1.0e-9))
    {
        return results;
    }
    const auto point_at = [&](double parameter)
    {
        return VpPoint2d{line.start_point.x + (parameter * delta_x),
                         line.start_point.y + (parameter * delta_y)};
    };
    if (first_parameter > 1.0e-9)
    {
        VpEntityRecord first_result = source;
        std::get<VpLineEntity>(first_result.geometry).end_point = point_at(first_parameter);
        results.push_back(std::move(first_result));
    }
    if (second_parameter < 1.0 - 1.0e-9)
    {
        VpEntityRecord second_result = source;
        std::get<VpLineEntity>(second_result.geometry).start_point = point_at(second_parameter);
        results.push_back(std::move(second_result));
    }
    return results;
}

bool joinedLineEntity(const std::vector<VpEntityRecord>& sources, double tolerance,
                      VpEntityRecord& result)
{
    if (sources.size() < 2 || tolerance <= 0.0)
    {
        return false;
    }
    for (const VpEntityRecord& source : sources)
    {
        if (source.type != VpEntityType::Line)
        {
            return false;
        }
    }

    const auto& first_line = std::get<VpLineEntity>(sources.front().geometry);
    std::vector<VpPoint2d> vertices{first_line.start_point, first_line.end_point};
    std::vector<bool> is_used(sources.size(), false);
    is_used.front() = true;
    std::size_t used_count = 1;
    while (used_count < sources.size())
    {
        bool did_connect = false;
        for (std::size_t index = 1; index < sources.size(); ++index)
        {
            if (is_used[index])
            {
                continue;
            }
            const auto& line = std::get<VpLineEntity>(sources[index].geometry);
            if (distance(vertices.back(), line.start_point) <= tolerance)
            {
                vertices.push_back(line.end_point);
            }
            else if (distance(vertices.back(), line.end_point) <= tolerance)
            {
                vertices.push_back(line.start_point);
            }
            else if (distance(vertices.front(), line.end_point) <= tolerance)
            {
                vertices.insert(vertices.begin(), line.start_point);
            }
            else if (distance(vertices.front(), line.start_point) <= tolerance)
            {
                vertices.insert(vertices.begin(), line.end_point);
            }
            else
            {
                continue;
            }
            is_used[index] = true;
            ++used_count;
            did_connect = true;
            break;
        }
        if (!did_connect)
        {
            return false;
        }
    }

    const bool is_closed =
        vertices.size() > 3 && distance(vertices.front(), vertices.back()) <= tolerance;
    if (is_closed)
    {
        vertices.pop_back();
    }
    result = sources.front();
    result.type = VpEntityType::Polyline;
    result.geometry = VpPolylineEntity{std::move(vertices), is_closed};
    return true;
}

std::vector<VpEntityRecord> explodedEntityParts(const VpEntityRecord& source)
{
    if (source.type == VpEntityType::LinearDimension)
    {
        const auto& dimension = std::get<VpLinearDimensionEntity>(source.geometry);
        std::vector<VpEntityRecord> results;
        const auto append_line =
            [&results, &source](const VpPoint2d& start_point, const VpPoint2d& end_point)
        {
            VpEntityRecord part = source;
            part.type = VpEntityType::Line;
            part.geometry = VpLineEntity{start_point, end_point};
            results.push_back(std::move(part));
        };
        VpPoint2d text_position = dimension.dimension_line_point;
        if (dimension.dimension_type == VpDimensionType::Linear ||
            dimension.dimension_type == VpDimensionType::Aligned)
        {
            VpPoint2d first_dimension_point;
            VpPoint2d second_dimension_point;
            if (dimension.dimension_type == VpDimensionType::Linear)
            {
                const VpPoint2d midpoint{(dimension.first_point.x + dimension.second_point.x) * 0.5,
                                         (dimension.first_point.y + dimension.second_point.y) *
                                             0.5};
                const bool is_vertical = std::abs(dimension.dimension_line_point.x - midpoint.x) >
                                         std::abs(dimension.dimension_line_point.y - midpoint.y);
                first_dimension_point =
                    is_vertical
                        ? VpPoint2d{dimension.dimension_line_point.x, dimension.first_point.y}
                        : VpPoint2d{dimension.first_point.x, dimension.dimension_line_point.y};
                second_dimension_point =
                    is_vertical
                        ? VpPoint2d{dimension.dimension_line_point.x, dimension.second_point.y}
                        : VpPoint2d{dimension.second_point.x, dimension.dimension_line_point.y};
            }
            else
            {
                const double delta_x = dimension.second_point.x - dimension.first_point.x;
                const double delta_y = dimension.second_point.y - dimension.first_point.y;
                const double length = std::hypot(delta_x, delta_y);
                if (length <= 1.0e-9)
                {
                    return {};
                }
                const double normal_x = -delta_y / length;
                const double normal_y = delta_x / length;
                const double offset =
                    ((dimension.dimension_line_point.x - dimension.first_point.x) * normal_x) +
                    ((dimension.dimension_line_point.y - dimension.first_point.y) * normal_y);
                first_dimension_point = {dimension.first_point.x + (normal_x * offset),
                                         dimension.first_point.y + (normal_y * offset)};
                second_dimension_point = {dimension.second_point.x + (normal_x * offset),
                                          dimension.second_point.y + (normal_y * offset)};
            }
            append_line(dimension.first_point, first_dimension_point);
            append_line(dimension.second_point, second_dimension_point);
            append_line(first_dimension_point, second_dimension_point);
            text_position = {(first_dimension_point.x + second_dimension_point.x) * 0.5,
                             (first_dimension_point.y + second_dimension_point.y) * 0.5};
        }
        else if (dimension.dimension_type == VpDimensionType::Radius ||
                 dimension.dimension_type == VpDimensionType::Diameter)
        {
            VpPoint2d start = dimension.center_point;
            if (dimension.dimension_type == VpDimensionType::Diameter)
            {
                start = {(2.0 * dimension.center_point.x) - dimension.first_point.x,
                         (2.0 * dimension.center_point.y) - dimension.first_point.y};
            }
            append_line(start, dimension.first_point);
            append_line(dimension.first_point, dimension.dimension_line_point);
        }
        else if (dimension.dimension_type == VpDimensionType::Ordinate)
        {
            const bool horizontal =
                std::abs(dimension.dimension_line_point.x - dimension.first_point.x) >
                std::abs(dimension.dimension_line_point.y - dimension.first_point.y);
            const VpPoint2d elbow =
                horizontal ? VpPoint2d{dimension.dimension_line_point.x, dimension.first_point.y}
                           : VpPoint2d{dimension.first_point.x, dimension.dimension_line_point.y};
            append_line(dimension.first_point, elbow);
            append_line(elbow, dimension.dimension_line_point);
        }
        else
        {
            append_line(dimension.center_point, dimension.first_point);
            append_line(dimension.center_point, dimension.second_point);
            VpEntityRecord arc_part = source;
            arc_part.type = VpEntityType::Arc;
            const double radius = distance(dimension.center_point, dimension.dimension_line_point);
            const double start_angle =
                entityAngleDegrees(dimension.center_point, dimension.first_point);
            const double end_angle =
                entityAngleDegrees(dimension.center_point, dimension.second_point);
            arc_part.geometry = VpArcEntity{dimension.center_point, radius, start_angle, end_angle};
            results.push_back(std::move(arc_part));
            double span = std::fmod(end_angle - start_angle + 360.0, 360.0);
            if (span > 180.0)
            {
                span -= 360.0;
            }
            const double middle_angle = (start_angle + (span * 0.5)) * kPi / 180.0;
            text_position = {dimension.center_point.x + (radius * std::cos(middle_angle)),
                             dimension.center_point.y + (radius * std::sin(middle_angle))};
        }
        VpEntityRecord text_part = source;
        text_part.type = VpEntityType::Text;
        text_part.geometry = VpTextEntity{text_position, dimensionDefaultText(dimension)};
        results.push_back(std::move(text_part));
        return results;
    }
    std::vector<VpPoint2d> vertices;
    bool is_closed = false;
    if (source.type == VpEntityType::Polyline)
    {
        const auto& polyline = std::get<VpPolylineEntity>(source.geometry);
        std::vector<VpEntityRecord> results;
        if (polyline.vertices.size() < 2)
        {
            return results;
        }
        const std::size_t segment_count =
            polyline.is_closed ? polyline.vertices.size() : polyline.vertices.size() - 1;
        results.reserve(segment_count);
        for (std::size_t index = 0; index < segment_count; ++index)
        {
            const std::size_t end_index = (index + 1) % polyline.vertices.size();
            VpEntityRecord part = source;
            VpArcEntity arc;
            if (index < polyline.bulges.size() &&
                bulgeArc(polyline.vertices[index], polyline.vertices[end_index],
                         polyline.bulges[index], arc))
            {
                part.type = VpEntityType::Arc;
                part.geometry = arc;
            }
            else
            {
                part.type = VpEntityType::Line;
                part.geometry =
                    VpLineEntity{polyline.vertices[index], polyline.vertices[end_index]};
            }
            results.push_back(std::move(part));
        }
        return results;
    }
    else if (source.type == VpEntityType::Hatch)
    {
        vertices = std::get<VpHatchEntity>(source.geometry).boundary;
        is_closed = true;
    }
    else
    {
        return {};
    }
    std::vector<VpEntityRecord> results;
    if (vertices.size() < 2)
    {
        return results;
    }
    results.reserve((vertices.size() - 1) + (is_closed ? 1 : 0));
    for (std::size_t index = 1; index < vertices.size(); ++index)
    {
        VpEntityRecord part = source;
        part.type = VpEntityType::Line;
        part.geometry = VpLineEntity{vertices[index - 1], vertices[index]};
        results.push_back(std::move(part));
    }
    if (is_closed && vertices.size() > 2 && distance(vertices.back(), vertices.front()) > 1.0e-9)
    {
        VpEntityRecord part = source;
        part.type = VpEntityType::Line;
        part.geometry = VpLineEntity{vertices.back(), vertices.front()};
        results.push_back(std::move(part));
    }
    return results;
}

} // namespace Vp
