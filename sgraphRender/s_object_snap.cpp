#include "s_object_snap.h"

#include "s_cad_viewport_geometry.h"
#include "s_dimension_geometry.h"
#include "s_ellipse_geometry.h"
#include "s_spline_geometry.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace smartGraphics
{
namespace
{

constexpr double kPi = 3.14159265358979323846;
constexpr double kEpsilon = 1.0e-9;

struct SSnapSegment
{
    SPoint2d start;
    SPoint2d end;
};

struct SSnapCircle
{
    SPoint2d center;
    double radius = 0.0;
    bool is_arc = false;
    double start_angle = 0.0;
    double end_angle = 0.0;
    bool is_clockwise = false;
};

double normalizedAngle(double angle)
{
    double result = std::fmod(angle, 360.0);
    if (result < 0.0)
    {
        result += 360.0;
    }
    return result;
}

double pointAngle(const SPoint2d& center, const SPoint2d& point)
{
    return normalizedAngle(std::atan2(point.y - center.y, point.x - center.x) * 180.0 / kPi);
}

bool angleOnArc(double angle, double start_angle, double end_angle, bool is_clockwise)
{
    const double span = normalizedAngle(is_clockwise ? start_angle - end_angle
                                                      : end_angle - start_angle);
    const double offset = normalizedAngle(is_clockwise ? start_angle - angle
                                                        : angle - start_angle);
    return offset <= span + 1.0e-7;
}

bool pointOnCurve(const SSnapCircle& circle, const SPoint2d& point)
{
    return !circle.is_arc ||
           angleOnArc(pointAngle(circle.center, point), circle.start_angle, circle.end_angle,
                      circle.is_clockwise);
}

SPoint2d polarPoint(const SPoint2d& center, double radius, double angle)
{
    const double radians = angle * kPi / 180.0;
    return {center.x + (radius * std::cos(radians)), center.y + (radius * std::sin(radians))};
}

SPoint2d closestPointOnSegment(const SPoint2d& point, const SSnapSegment& segment)
{
    const double delta_x = segment.end.x - segment.start.x;
    const double delta_y = segment.end.y - segment.start.y;
    const double length_squared = (delta_x * delta_x) + (delta_y * delta_y);
    if (length_squared <= kEpsilon)
    {
        return segment.start;
    }
    const double parameter = std::clamp(
        (((point.x - segment.start.x) * delta_x) + ((point.y - segment.start.y) * delta_y)) /
            length_squared,
        0.0, 1.0);
    return {segment.start.x + (parameter * delta_x), segment.start.y + (parameter * delta_y)};
}

double infiniteLineParameter(const SPoint2d& point, const SSnapSegment& segment)
{
    const double delta_x = segment.end.x - segment.start.x;
    const double delta_y = segment.end.y - segment.start.y;
    const double length_squared = (delta_x * delta_x) + (delta_y * delta_y);
    return length_squared <= kEpsilon ? 0.0
                                      : (((point.x - segment.start.x) * delta_x) +
                                         ((point.y - segment.start.y) * delta_y)) /
                                            length_squared;
}

SPoint2d pointOnInfiniteLine(const SSnapSegment& segment, double parameter)
{
    return {segment.start.x + ((segment.end.x - segment.start.x) * parameter),
            segment.start.y + ((segment.end.y - segment.start.y) * parameter)};
}

bool infiniteLineIntersection(const SSnapSegment& first, const SSnapSegment& second,
                              SPoint2d& intersection, double& first_parameter,
                              double& second_parameter)
{
    const double first_x = first.end.x - first.start.x;
    const double first_y = first.end.y - first.start.y;
    const double second_x = second.end.x - second.start.x;
    const double second_y = second.end.y - second.start.y;
    const double denominator = (first_x * second_y) - (first_y * second_x);
    if (std::abs(denominator) <= kEpsilon)
    {
        return false;
    }
    const double origin_x = second.start.x - first.start.x;
    const double origin_y = second.start.y - first.start.y;
    first_parameter = ((origin_x * second_y) - (origin_y * second_x)) / denominator;
    second_parameter = ((origin_x * first_y) - (origin_y * first_x)) / denominator;
    intersection = {first.start.x + (first_parameter * first_x),
                    first.start.y + (first_parameter * first_y)};
    return true;
}

bool segmentIntersection(const SSnapSegment& first, const SSnapSegment& second,
                         SPoint2d& intersection)
{
    const double first_x = first.end.x - first.start.x;
    const double first_y = first.end.y - first.start.y;
    const double second_x = second.end.x - second.start.x;
    const double second_y = second.end.y - second.start.y;
    const double denominator = (first_x * second_y) - (first_y * second_x);
    if (std::abs(denominator) <= kEpsilon)
    {
        return false;
    }
    const double origin_x = second.start.x - first.start.x;
    const double origin_y = second.start.y - first.start.y;
    const double first_parameter = ((origin_x * second_y) - (origin_y * second_x)) / denominator;
    const double second_parameter = ((origin_x * first_y) - (origin_y * first_x)) / denominator;
    if (first_parameter < 0.0 || first_parameter > 1.0 || second_parameter < 0.0 ||
        second_parameter > 1.0)
    {
        return false;
    }
    intersection = {first.start.x + (first_parameter * first_x),
                    first.start.y + (first_parameter * first_y)};
    return true;
}

std::vector<SPoint2d> segmentCircleIntersections(const SSnapSegment& segment,
                                                 const SSnapCircle& circle)
{
    const double delta_x = segment.end.x - segment.start.x;
    const double delta_y = segment.end.y - segment.start.y;
    const double offset_x = segment.start.x - circle.center.x;
    const double offset_y = segment.start.y - circle.center.y;
    const double a = (delta_x * delta_x) + (delta_y * delta_y);
    const double b = 2.0 * ((offset_x * delta_x) + (offset_y * delta_y));
    const double c =
        (offset_x * offset_x) + (offset_y * offset_y) - (circle.radius * circle.radius);
    const double discriminant = (b * b) - (4.0 * a * c);
    std::vector<SPoint2d> results;
    if (a <= kEpsilon || discriminant < -kEpsilon)
    {
        return results;
    }
    const double root = std::sqrt(std::max(0.0, discriminant));
    const std::array<double, 2> parameters{(-b - root) / (2.0 * a), (-b + root) / (2.0 * a)};
    for (double parameter : parameters)
    {
        if (parameter >= 0.0 && parameter <= 1.0)
        {
            const SPoint2d point{segment.start.x + (parameter * delta_x),
                                 segment.start.y + (parameter * delta_y)};
            if (pointOnCurve(circle, point) &&
                (results.empty() || distance(results.front(), point) > kEpsilon))
            {
                results.push_back(point);
            }
        }
    }
    return results;
}

std::vector<SPoint2d> circleIntersections(const SSnapCircle& first, const SSnapCircle& second)
{
    std::vector<SPoint2d> results;
    const double center_distance = distance(first.center, second.center);
    if (center_distance <= kEpsilon || center_distance > first.radius + second.radius + kEpsilon ||
        center_distance < std::abs(first.radius - second.radius) - kEpsilon)
    {
        return results;
    }
    const double along = ((first.radius * first.radius) - (second.radius * second.radius) +
                          (center_distance * center_distance)) /
                         (2.0 * center_distance);
    const double height_squared = (first.radius * first.radius) - (along * along);
    const double height = std::sqrt(std::max(0.0, height_squared));
    const double unit_x = (second.center.x - first.center.x) / center_distance;
    const double unit_y = (second.center.y - first.center.y) / center_distance;
    const SPoint2d base{first.center.x + (along * unit_x), first.center.y + (along * unit_y)};
    const std::array<SPoint2d, 2> candidates{
        SPoint2d{base.x - (height * unit_y), base.y + (height * unit_x)},
        SPoint2d{base.x + (height * unit_y), base.y - (height * unit_x)},
    };
    for (const SPoint2d& point : candidates)
    {
        if (pointOnCurve(first, point) && pointOnCurve(second, point) &&
            (results.empty() || distance(results.front(), point) > kEpsilon))
        {
            results.push_back(point);
        }
    }
    return results;
}

void appendEntityGeometry(const SEntityRecord& entity, std::vector<SSnapSegment>& segments,
                          std::vector<SSnapCircle>& circles)
{
    if (entity.type == SEntityType::Line)
    {
        const auto& line = std::get<SLineEntity>(entity.geometry);
        segments.push_back({line.start_point, line.end_point});
    }
    else if (entity.type == SEntityType::Polyline)
    {
        const auto& polyline = std::get<SPolylineEntity>(entity.geometry);
        if (polyline.vertices.size() < 2)
        {
            return;
        }
        const std::size_t segment_count =
            polyline.is_closed ? polyline.vertices.size() : polyline.vertices.size() - 1;
        for (std::size_t index = 0; index < segment_count; ++index)
        {
            const std::size_t end_index = (index + 1) % polyline.vertices.size();
            SArcEntity arc;
            if (index < polyline.bulges.size() &&
                bulgeArc(polyline.vertices[index], polyline.vertices[end_index],
                         polyline.bulges[index], arc))
            {
                circles.push_back(
                    {arc.center, arc.radius, true, arc.start_angle, arc.end_angle, false});
            }
            else
            {
                segments.push_back({polyline.vertices[index], polyline.vertices[end_index]});
            }
        }
    }
    else if (entity.type == SEntityType::Circle)
    {
        const auto& circle = std::get<SCircleEntity>(entity.geometry);
        circles.push_back({circle.center, circle.radius, false, 0.0, 0.0, false});
    }
    else if (entity.type == SEntityType::Arc)
    {
        const auto& arc = std::get<SArcEntity>(entity.geometry);
        circles.push_back(
            {arc.center, arc.radius, true, arc.start_angle, arc.end_angle, arc.is_clockwise});
    }
    else if (entity.type == SEntityType::LinearDimension)
    {
        const auto& dimension = std::get<SLinearDimensionEntity>(entity.geometry);
        const std::vector<SPoint2d> points = dimensionReferencePoints(dimension);
        for (std::size_t index = 1; index < points.size(); ++index)
        {
            segments.push_back({points[index - 1], points[index]});
        }
    }
    else if (entity.type == SEntityType::Hatch)
    {
        const auto& boundary = std::get<SHatchEntity>(entity.geometry).boundary;
        for (std::size_t index = 1; index < boundary.size(); ++index)
        {
            segments.push_back({boundary[index - 1], boundary[index]});
        }
        if (boundary.size() > 2)
        {
            segments.push_back({boundary.back(), boundary.front()});
        }
    }
}

} // namespace

std::optional<SObjectSnapResult> findObjectSnap(const std::vector<const SEntityRecord*>& entities,
                                                const SPoint2d& cursor,
                                                const std::optional<SPoint2d>& reference_point,
                                                double tolerance, SObjectSnapModes modes)
{
    std::optional<SObjectSnapResult> best_result;
    double best_distance = tolerance;
    int best_priority = -1;
    const auto consider = [&](const SPoint2d& point, SObjectSnapType type, int priority)
    {
        if ((type == SObjectSnapType::Endpoint &&
             !objectSnapModeEnabled(modes, SObjectSnapMode::Endpoint)) ||
            (type == SObjectSnapType::Center &&
             !objectSnapModeEnabled(modes, SObjectSnapMode::Center)))
        {
            return;
        }
        const double candidate_distance = distance(cursor, point);
        if (candidate_distance > tolerance)
        {
            return;
        }
        if (priority > best_priority ||
            (priority == best_priority && candidate_distance < best_distance - kEpsilon))
        {
            best_result = SObjectSnapResult{point, type};
            best_distance = candidate_distance;
            best_priority = priority;
        }
    };

    std::vector<SSnapSegment> nearby_segments;
    std::vector<SSnapCircle> nearby_circles;
    for (const SEntityRecord* entity : entities)
    {
        if (!entity)
        {
            continue;
        }
        if (entity->type == SEntityType::Text)
        {
            consider(std::get<STextEntity>(entity->geometry).position, SObjectSnapType::Endpoint,
                     80);
        }
        if (entity->type == SEntityType::MText)
        {
            consider(std::get<SMTextEntity>(entity->geometry).position, SObjectSnapType::Endpoint,
                     80);
        }
        if (entity->type == SEntityType::Leader)
        {
            const auto& vertices = std::get<SLeaderEntity>(entity->geometry).vertices;
            for (std::size_t index = 0; index < vertices.size(); ++index)
            {
                consider(vertices[index], SObjectSnapType::Endpoint, 80);
                if (index > 0)
                {
                    consider({(vertices[index - 1].x + vertices[index].x) * 0.5,
                              (vertices[index - 1].y + vertices[index].y) * 0.5},
                             SObjectSnapType::Midpoint, 70);
                }
            }
        }
        if (entity->type == SEntityType::Spline)
        {
            const auto& spline = std::get<SSplineEntity>(entity->geometry);
            consider(spline.control_points.front(), SObjectSnapType::Endpoint, 80);
            consider(spline.control_points.back(), SObjectSnapType::Endpoint, 80);
            consider(splinePoint(spline, 0.5), SObjectSnapType::Midpoint, 70);
            for (const SPoint2d& point : splineApproximation(spline, 96))
            {
                consider(point, SObjectSnapType::Nearest, 10);
            }
            continue;
        }
        if (entity->type == SEntityType::Ellipse)
        {
            const auto& ellipse = std::get<SEllipseEntity>(entity->geometry);
            consider(ellipse.center, SObjectSnapType::Center, 90);
            consider(
                {ellipse.center.x + ellipse.major_axis.x, ellipse.center.y + ellipse.major_axis.y},
                SObjectSnapType::Quadrant, 75);
            consider(
                {ellipse.center.x - ellipse.major_axis.x, ellipse.center.y - ellipse.major_axis.y},
                SObjectSnapType::Quadrant, 75);
            consider(
                {ellipse.center.x + ellipse.minor_axis.x, ellipse.center.y + ellipse.minor_axis.y},
                SObjectSnapType::Quadrant, 75);
            consider(
                {ellipse.center.x - ellipse.minor_axis.x, ellipse.center.y - ellipse.minor_axis.y},
                SObjectSnapType::Quadrant, 75);
            for (const SPoint2d& point : ellipseApproximation(ellipse, 192))
            {
                consider(point, SObjectSnapType::Nearest, 10);
            }
            continue;
        }
        std::vector<SSnapSegment> entity_segments;
        std::vector<SSnapCircle> entity_circles;
        appendEntityGeometry(*entity, entity_segments, entity_circles);
        for (const SSnapSegment& segment : entity_segments)
        {
            consider(segment.start, SObjectSnapType::Endpoint, 80);
            consider(segment.end, SObjectSnapType::Endpoint, 80);
            consider(
                {(segment.start.x + segment.end.x) * 0.5, (segment.start.y + segment.end.y) * 0.5},
                SObjectSnapType::Midpoint, 70);
            consider(closestPointOnSegment(cursor, segment), SObjectSnapType::Nearest, 10);
            const double extension_parameter = infiniteLineParameter(cursor, segment);
            const SPoint2d extension_point = pointOnInfiniteLine(segment, extension_parameter);
            if (extension_parameter < 0.0 || extension_parameter > 1.0)
            {
                consider(extension_point, SObjectSnapType::Extension, 50);
            }
            if (reference_point)
            {
                consider(closestPointOnSegment(*reference_point, segment),
                         SObjectSnapType::Perpendicular, 60);
                const double direction_x = segment.end.x - segment.start.x;
                const double direction_y = segment.end.y - segment.start.y;
                const double length_squared =
                    (direction_x * direction_x) + (direction_y * direction_y);
                if (length_squared > kEpsilon)
                {
                    const double parallel_parameter =
                        (((cursor.x - reference_point->x) * direction_x) +
                         ((cursor.y - reference_point->y) * direction_y)) /
                        length_squared;
                    consider({reference_point->x + (parallel_parameter * direction_x),
                              reference_point->y + (parallel_parameter * direction_y)},
                             SObjectSnapType::Parallel, 55);
                }
            }
            if (distance(cursor, extension_point) <= tolerance * 1.5 && nearby_segments.size() < 64)
            {
                nearby_segments.push_back(segment);
            }
        }
        for (const SSnapCircle& circle : entity_circles)
        {
            consider(circle.center, SObjectSnapType::Center, 65);
            const std::array<double, 4> quadrant_angles{0.0, 90.0, 180.0, 270.0};
            for (double angle : quadrant_angles)
            {
                const SPoint2d point = polarPoint(circle.center, circle.radius, angle);
                if (pointOnCurve(circle, point))
                {
                    consider(point, SObjectSnapType::Quadrant, 70);
                }
            }
            if (circle.is_arc)
            {
                consider(polarPoint(circle.center, circle.radius, circle.start_angle),
                         SObjectSnapType::Endpoint, 80);
                consider(polarPoint(circle.center, circle.radius, circle.end_angle),
                         SObjectSnapType::Endpoint, 80);
                const double midpoint_angle =
                    circle.start_angle + (circle.is_clockwise ? -1.0 : 1.0) *
                                             (normalizedAngle(circle.is_clockwise
                                                                  ? circle.start_angle -
                                                                        circle.end_angle
                                                                  : circle.end_angle -
                                                                        circle.start_angle) *
                                              0.5);
                consider(polarPoint(circle.center, circle.radius, midpoint_angle),
                         SObjectSnapType::Midpoint, 70);
            }
            if (distance(cursor, circle.center) > kEpsilon)
            {
                const SPoint2d nearest =
                    polarPoint(circle.center, circle.radius, pointAngle(circle.center, cursor));
                if (pointOnCurve(circle, nearest))
                {
                    consider(nearest, SObjectSnapType::Nearest, 10);
                }
            }
            if (reference_point)
            {
                const double reference_distance = distance(circle.center, *reference_point);
                if (reference_distance > kEpsilon)
                {
                    const double reference_angle = pointAngle(circle.center, *reference_point);
                    const std::array<SPoint2d, 2> perpendicular_points{
                        polarPoint(circle.center, circle.radius, reference_angle),
                        polarPoint(circle.center, circle.radius, reference_angle + 180.0),
                    };
                    for (const SPoint2d& point : perpendicular_points)
                    {
                        if (pointOnCurve(circle, point))
                        {
                            consider(point, SObjectSnapType::Perpendicular, 60);
                        }
                    }
                }
                if (reference_distance > circle.radius + kEpsilon)
                {
                    const double base_angle = pointAngle(circle.center, *reference_point);
                    const double tangent_offset =
                        std::acos(circle.radius / reference_distance) * 180.0 / kPi;
                    const std::array<SPoint2d, 2> tangent_points{
                        polarPoint(circle.center, circle.radius, base_angle - tangent_offset),
                        polarPoint(circle.center, circle.radius, base_angle + tangent_offset),
                    };
                    for (const SPoint2d& point : tangent_points)
                    {
                        if (pointOnCurve(circle, point))
                        {
                            consider(point, SObjectSnapType::Tangent, 60);
                        }
                    }
                }
            }
            if (std::abs(distance(cursor, circle.center) - circle.radius) <= tolerance * 1.5 &&
                nearby_circles.size() < 64)
            {
                nearby_circles.push_back(circle);
            }
        }
    }

    if (!nearby_segments.empty() || !nearby_circles.empty())
    {
        for (std::size_t first = 0; first < nearby_segments.size(); ++first)
        {
            for (std::size_t second = first + 1; second < nearby_segments.size(); ++second)
            {
                SPoint2d point;
                if (segmentIntersection(nearby_segments[first], nearby_segments[second], point))
                {
                    consider(point, SObjectSnapType::Intersection, 100);
                }
                else
                {
                    double first_parameter = 0.0;
                    double second_parameter = 0.0;
                    if (infiniteLineIntersection(nearby_segments[first], nearby_segments[second],
                                                 point, first_parameter, second_parameter) &&
                        (first_parameter < 0.0 || first_parameter > 1.0 || second_parameter < 0.0 ||
                         second_parameter > 1.0))
                    {
                        consider(point, SObjectSnapType::ApparentIntersection, 90);
                    }
                }
            }
            for (const SSnapCircle& circle : nearby_circles)
            {
                for (const SPoint2d& point :
                     segmentCircleIntersections(nearby_segments[first], circle))
                {
                    consider(point, SObjectSnapType::Intersection, 100);
                }
            }
        }
        for (std::size_t first = 0; first < nearby_circles.size(); ++first)
        {
            for (std::size_t second = first + 1; second < nearby_circles.size(); ++second)
            {
                for (const SPoint2d& point :
                     circleIntersections(nearby_circles[first], nearby_circles[second]))
                {
                    consider(point, SObjectSnapType::Intersection, 100);
                }
            }
        }
    }
    return best_result;
}

} // namespace smartGraphics
