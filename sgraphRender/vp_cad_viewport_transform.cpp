#include "vp_cad_viewport_geometry.h"

#include <cmath>

namespace Vp
{
namespace
{

constexpr double kPi = 3.14159265358979323846;

VpPoint2d rotatePoint(const VpPoint2d& point, const VpPoint2d& center, double angle)
{
    const double radians = angle * kPi / 180.0;
    const double sine = std::sin(radians);
    const double cosine = std::cos(radians);
    const double delta_x = point.x - center.x;
    const double delta_y = point.y - center.y;
    return {center.x + (delta_x * cosine) - (delta_y * sine),
            center.y + (delta_x * sine) + (delta_y * cosine)};
}

VpPoint2d scalePoint(const VpPoint2d& point, const VpPoint2d& base_point, double scale_factor)
{
    return {base_point.x + ((point.x - base_point.x) * scale_factor),
            base_point.y + ((point.y - base_point.y) * scale_factor)};
}

VpPoint2d mirrorPoint(const VpPoint2d& point, const VpPoint2d& axis_start,
                      const VpPoint2d& axis_end)
{
    const double axis_x = axis_end.x - axis_start.x;
    const double axis_y = axis_end.y - axis_start.y;
    const double length_squared = (axis_x * axis_x) + (axis_y * axis_y);
    if (length_squared <= 1.0e-18)
    {
        return point;
    }
    const double projection =
        (((point.x - axis_start.x) * axis_x) + ((point.y - axis_start.y) * axis_y)) /
        length_squared;
    const VpPoint2d projected{axis_start.x + (projection * axis_x),
                              axis_start.y + (projection * axis_y)};
    return {(2.0 * projected.x) - point.x, (2.0 * projected.y) - point.y};
}

template <typename SFunction>
void transformSpline(VpEntityRecord& entity, SFunction transform_point)
{
    for (VpPoint2d& point : std::get<VpSplineEntity>(entity.geometry).control_points)
    {
        transform_point(point);
    }
}

template <typename SFunction>
void transformEllipse(VpEntityRecord& entity, SFunction transform_point)
{
    auto& ellipse = std::get<VpEllipseEntity>(entity.geometry);
    VpPoint2d major_endpoint{ellipse.center.x + ellipse.major_axis.x,
                             ellipse.center.y + ellipse.major_axis.y};
    VpPoint2d minor_endpoint{ellipse.center.x + ellipse.minor_axis.x,
                             ellipse.center.y + ellipse.minor_axis.y};
    transform_point(major_endpoint);
    transform_point(minor_endpoint);
    transform_point(ellipse.center);
    ellipse.major_axis = {major_endpoint.x - ellipse.center.x, major_endpoint.y - ellipse.center.y};
    ellipse.minor_axis = {minor_endpoint.x - ellipse.center.x, minor_endpoint.y - ellipse.center.y};
}

} // namespace

VpEntityRecord translatedEntity(const VpEntityRecord& source, double delta_x, double delta_y)
{
    VpEntityRecord result = source;
    const auto translate_point = [delta_x, delta_y](VpPoint2d& point)
    {
        point.x += delta_x;
        point.y += delta_y;
    };
    if (result.type == VpEntityType::Line)
    {
        auto& line = std::get<VpLineEntity>(result.geometry);
        translate_point(line.start_point);
        translate_point(line.end_point);
    }
    else if (result.type == VpEntityType::Circle)
    {
        translate_point(std::get<VpCircleEntity>(result.geometry).center);
    }
    else if (result.type == VpEntityType::Arc)
    {
        translate_point(std::get<VpArcEntity>(result.geometry).center);
    }
    else if (result.type == VpEntityType::Polyline)
    {
        for (VpPoint2d& vertex : std::get<VpPolylineEntity>(result.geometry).vertices)
        {
            translate_point(vertex);
        }
    }
    else if (result.type == VpEntityType::Text)
    {
        translate_point(std::get<VpTextEntity>(result.geometry).position);
    }
    else if (result.type == VpEntityType::LinearDimension)
    {
        auto& dimension = std::get<VpLinearDimensionEntity>(result.geometry);
        translate_point(dimension.first_point);
        translate_point(dimension.second_point);
        translate_point(dimension.dimension_line_point);
        translate_point(dimension.center_point);
    }
    else if (result.type == VpEntityType::Hatch)
    {
        auto& hatch = std::get<VpHatchEntity>(result.geometry);
        for (VpPoint2d& vertex : hatch.boundary)
        {
            translate_point(vertex);
        }
        for (std::vector<VpPoint2d>& island : hatch.island_boundaries)
        {
            for (VpPoint2d& vertex : island)
            {
                translate_point(vertex);
            }
        }
    }
    else if (result.type == VpEntityType::Spline)
    {
        transformSpline(result, translate_point);
    }
    else if (result.type == VpEntityType::Ellipse)
    {
        transformEllipse(result, translate_point);
    }
    else if (result.type == VpEntityType::MText)
    {
        translate_point(std::get<VpMTextEntity>(result.geometry).position);
    }
    else if (result.type == VpEntityType::Leader)
    {
        for (VpPoint2d& point : std::get<VpLeaderEntity>(result.geometry).vertices)
        {
            translate_point(point);
        }
    }
    if (result.associative_array)
    {
        if (result.associative_array->array_type == VpArrayType::Path)
        {
            result.associative_array.reset();
        }
        else
        {
            VpEntityRecord source_geometry;
            source_geometry.type = result.associative_array->source_type;
            source_geometry.geometry = result.associative_array->source_geometry;
            result.associative_array->source_geometry =
                translatedEntity(source_geometry, delta_x, delta_y).geometry;
            translate_point(result.associative_array->center);
            translate_point(result.associative_array->source_base_point);
        }
    }
    return result;
}

VpEntityRecord rotatedEntity(const VpEntityRecord& source, const VpPoint2d& center, double angle)
{
    VpEntityRecord result = source;
    const auto rotate_point = [&center, angle](VpPoint2d& point)
    {
        point = rotatePoint(point, center, angle);
    };
    if (result.type == VpEntityType::Line)
    {
        auto& line = std::get<VpLineEntity>(result.geometry);
        rotate_point(line.start_point);
        rotate_point(line.end_point);
    }
    else if (result.type == VpEntityType::Circle)
    {
        rotate_point(std::get<VpCircleEntity>(result.geometry).center);
    }
    else if (result.type == VpEntityType::Arc)
    {
        auto& arc = std::get<VpArcEntity>(result.geometry);
        rotate_point(arc.center);
        arc.start_angle += angle;
        arc.end_angle += angle;
    }
    else if (result.type == VpEntityType::Polyline)
    {
        for (VpPoint2d& vertex : std::get<VpPolylineEntity>(result.geometry).vertices)
        {
            rotate_point(vertex);
        }
    }
    else if (result.type == VpEntityType::Text)
    {
        auto& text = std::get<VpTextEntity>(result.geometry);
        rotate_point(text.position);
        text.rotation += angle;
    }
    else if (result.type == VpEntityType::LinearDimension)
    {
        auto& dimension = std::get<VpLinearDimensionEntity>(result.geometry);
        rotate_point(dimension.first_point);
        rotate_point(dimension.second_point);
        rotate_point(dimension.dimension_line_point);
        rotate_point(dimension.center_point);
    }
    else if (result.type == VpEntityType::Hatch)
    {
        auto& hatch = std::get<VpHatchEntity>(result.geometry);
        for (VpPoint2d& vertex : hatch.boundary)
        {
            rotate_point(vertex);
        }
        for (std::vector<VpPoint2d>& island : hatch.island_boundaries)
        {
            for (VpPoint2d& vertex : island)
            {
                rotate_point(vertex);
            }
        }
    }
    else if (result.type == VpEntityType::Spline)
    {
        transformSpline(result, rotate_point);
    }
    else if (result.type == VpEntityType::Ellipse)
    {
        transformEllipse(result, rotate_point);
    }
    else if (result.type == VpEntityType::MText)
    {
        auto& text = std::get<VpMTextEntity>(result.geometry);
        rotate_point(text.position);
        text.rotation += angle;
    }
    else if (result.type == VpEntityType::Leader)
    {
        for (VpPoint2d& point : std::get<VpLeaderEntity>(result.geometry).vertices)
        {
            rotate_point(point);
        }
    }
    return result;
}

VpEntityRecord scaledEntity(const VpEntityRecord& source, const VpPoint2d& base_point,
                            double scale_factor)
{
    VpEntityRecord result = source;
    const auto scale_point = [&base_point, scale_factor](VpPoint2d& point)
    {
        point = scalePoint(point, base_point, scale_factor);
    };
    if (result.type == VpEntityType::Circle)
    {
        auto& circle = std::get<VpCircleEntity>(result.geometry);
        scale_point(circle.center);
        circle.radius *= std::abs(scale_factor);
    }
    else if (result.type == VpEntityType::Arc)
    {
        auto& arc = std::get<VpArcEntity>(result.geometry);
        scale_point(arc.center);
        arc.radius *= std::abs(scale_factor);
    }
    else if (result.type == VpEntityType::Text)
    {
        auto& text = std::get<VpTextEntity>(result.geometry);
        scale_point(text.position);
        text.height *= std::abs(scale_factor);
    }
    else if (result.type == VpEntityType::Line)
    {
        auto& line = std::get<VpLineEntity>(result.geometry);
        scale_point(line.start_point);
        scale_point(line.end_point);
    }
    else if (result.type == VpEntityType::Polyline)
    {
        for (VpPoint2d& point : std::get<VpPolylineEntity>(result.geometry).vertices)
        {
            scale_point(point);
        }
        auto& polyline = std::get<VpPolylineEntity>(result.geometry);
        for (double& width : polyline.start_widths)
        {
            width *= std::abs(scale_factor);
        }
        for (double& width : polyline.end_widths)
        {
            width *= std::abs(scale_factor);
        }
    }
    else if (result.type == VpEntityType::LinearDimension)
    {
        auto& dimension = std::get<VpLinearDimensionEntity>(result.geometry);
        scale_point(dimension.first_point);
        scale_point(dimension.second_point);
        scale_point(dimension.dimension_line_point);
        scale_point(dimension.center_point);
    }
    else if (result.type == VpEntityType::Hatch)
    {
        auto& hatch = std::get<VpHatchEntity>(result.geometry);
        for (VpPoint2d& point : hatch.boundary)
        {
            scale_point(point);
        }
        for (std::vector<VpPoint2d>& island : hatch.island_boundaries)
        {
            for (VpPoint2d& point : island)
            {
                scale_point(point);
            }
        }
    }
    else if (result.type == VpEntityType::Spline)
    {
        transformSpline(result, scale_point);
    }
    else if (result.type == VpEntityType::Ellipse)
    {
        transformEllipse(result, scale_point);
    }
    else if (result.type == VpEntityType::MText)
    {
        auto& text = std::get<VpMTextEntity>(result.geometry);
        scale_point(text.position);
        text.width *= std::abs(scale_factor);
        text.height *= std::abs(scale_factor);
    }
    else if (result.type == VpEntityType::Leader)
    {
        auto& leader = std::get<VpLeaderEntity>(result.geometry);
        for (VpPoint2d& point : leader.vertices)
        {
            scale_point(point);
        }
        leader.text_height *= std::abs(scale_factor);
        leader.arrow_size *= std::abs(scale_factor);
    }
    return result;
}

VpEntityRecord mirroredEntity(const VpEntityRecord& source, const VpPoint2d& axis_start,
                              const VpPoint2d& axis_end)
{
    VpEntityRecord result = source;
    const auto mirror_point = [&axis_start, &axis_end](VpPoint2d& point)
    {
        point = mirrorPoint(point, axis_start, axis_end);
    };
    if (result.type == VpEntityType::Arc)
    {
        auto& arc = std::get<VpArcEntity>(result.geometry);
        const double axis_angle =
            std::atan2(axis_end.y - axis_start.y, axis_end.x - axis_start.x) * 180.0 / kPi;
        mirror_point(arc.center);
        arc.start_angle = (2.0 * axis_angle) - arc.start_angle;
        arc.end_angle = (2.0 * axis_angle) - arc.end_angle;
        arc.is_clockwise = !arc.is_clockwise;
    }
    else if (result.type == VpEntityType::Text)
    {
        auto& text = std::get<VpTextEntity>(result.geometry);
        const double axis_angle =
            std::atan2(axis_end.y - axis_start.y, axis_end.x - axis_start.x) * 180.0 / kPi;
        mirror_point(text.position);
        text.rotation = (2.0 * axis_angle) - text.rotation;
    }
    else if (result.type == VpEntityType::Polyline)
    {
        auto& polyline = std::get<VpPolylineEntity>(result.geometry);
        for (VpPoint2d& point : polyline.vertices)
        {
            mirror_point(point);
        }
        for (double& bulge : polyline.bulges)
        {
            bulge = -bulge;
        }
    }
    else if (result.type == VpEntityType::Line)
    {
        auto& line = std::get<VpLineEntity>(result.geometry);
        mirror_point(line.start_point);
        mirror_point(line.end_point);
    }
    else if (result.type == VpEntityType::Circle)
    {
        mirror_point(std::get<VpCircleEntity>(result.geometry).center);
    }
    else if (result.type == VpEntityType::LinearDimension)
    {
        auto& dimension = std::get<VpLinearDimensionEntity>(result.geometry);
        mirror_point(dimension.first_point);
        mirror_point(dimension.second_point);
        mirror_point(dimension.dimension_line_point);
        mirror_point(dimension.center_point);
    }
    else if (result.type == VpEntityType::Hatch)
    {
        auto& hatch = std::get<VpHatchEntity>(result.geometry);
        for (VpPoint2d& point : hatch.boundary)
        {
            mirror_point(point);
        }
        for (std::vector<VpPoint2d>& island : hatch.island_boundaries)
        {
            for (VpPoint2d& point : island)
            {
                mirror_point(point);
            }
        }
    }
    else if (result.type == VpEntityType::Spline)
    {
        transformSpline(result, mirror_point);
    }
    else if (result.type == VpEntityType::Ellipse)
    {
        transformEllipse(result, mirror_point);
    }
    else if (result.type == VpEntityType::MText)
    {
        auto& text = std::get<VpMTextEntity>(result.geometry);
        const double axis_angle =
            std::atan2(axis_end.y - axis_start.y, axis_end.x - axis_start.x) * 180.0 / kPi;
        mirror_point(text.position);
        text.rotation = (2.0 * axis_angle) - text.rotation;
    }
    else if (result.type == VpEntityType::Leader)
    {
        for (VpPoint2d& point : std::get<VpLeaderEntity>(result.geometry).vertices)
        {
            mirror_point(point);
        }
    }
    return result;
}

} // namespace Vp
