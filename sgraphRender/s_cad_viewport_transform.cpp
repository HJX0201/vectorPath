#include "s_cad_viewport_geometry.h"

#include <cmath>

namespace smartGraphics
{
namespace
{

constexpr double kPi = 3.14159265358979323846;

SPoint2d rotatePoint(const SPoint2d& point, const SPoint2d& center, double angle)
{
    const double radians = angle * kPi / 180.0;
    const double sine = std::sin(radians);
    const double cosine = std::cos(radians);
    const double delta_x = point.x - center.x;
    const double delta_y = point.y - center.y;
    return {center.x + (delta_x * cosine) - (delta_y * sine),
            center.y + (delta_x * sine) + (delta_y * cosine)};
}

SPoint2d scalePoint(const SPoint2d& point, const SPoint2d& base_point, double scale_factor)
{
    return {base_point.x + ((point.x - base_point.x) * scale_factor),
            base_point.y + ((point.y - base_point.y) * scale_factor)};
}

SPoint2d mirrorPoint(const SPoint2d& point, const SPoint2d& axis_start, const SPoint2d& axis_end)
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
    const SPoint2d projected{axis_start.x + (projection * axis_x),
                             axis_start.y + (projection * axis_y)};
    return {(2.0 * projected.x) - point.x, (2.0 * projected.y) - point.y};
}

template <typename SFunction> void transformSpline(SEntityRecord& entity, SFunction transform_point)
{
    for (SPoint2d& point : std::get<SSplineEntity>(entity.geometry).control_points)
    {
        transform_point(point);
    }
}

template <typename SFunction>
void transformEllipse(SEntityRecord& entity, SFunction transform_point)
{
    auto& ellipse = std::get<SEllipseEntity>(entity.geometry);
    SPoint2d major_endpoint{ellipse.center.x + ellipse.major_axis.x,
                            ellipse.center.y + ellipse.major_axis.y};
    SPoint2d minor_endpoint{ellipse.center.x + ellipse.minor_axis.x,
                            ellipse.center.y + ellipse.minor_axis.y};
    transform_point(major_endpoint);
    transform_point(minor_endpoint);
    transform_point(ellipse.center);
    ellipse.major_axis = {major_endpoint.x - ellipse.center.x, major_endpoint.y - ellipse.center.y};
    ellipse.minor_axis = {minor_endpoint.x - ellipse.center.x, minor_endpoint.y - ellipse.center.y};
}

} // namespace

SEntityRecord translatedEntity(const SEntityRecord& source, double delta_x, double delta_y)
{
    SEntityRecord result = source;
    const auto translate_point = [delta_x, delta_y](SPoint2d& point)
    {
        point.x += delta_x;
        point.y += delta_y;
    };
    if (result.type == SEntityType::Line)
    {
        auto& line = std::get<SLineEntity>(result.geometry);
        translate_point(line.start_point);
        translate_point(line.end_point);
    }
    else if (result.type == SEntityType::Circle)
    {
        translate_point(std::get<SCircleEntity>(result.geometry).center);
    }
    else if (result.type == SEntityType::Arc)
    {
        translate_point(std::get<SArcEntity>(result.geometry).center);
    }
    else if (result.type == SEntityType::Polyline)
    {
        for (SPoint2d& vertex : std::get<SPolylineEntity>(result.geometry).vertices)
        {
            translate_point(vertex);
        }
    }
    else if (result.type == SEntityType::Text)
    {
        translate_point(std::get<STextEntity>(result.geometry).position);
    }
    else if (result.type == SEntityType::LinearDimension)
    {
        auto& dimension = std::get<SLinearDimensionEntity>(result.geometry);
        translate_point(dimension.first_point);
        translate_point(dimension.second_point);
        translate_point(dimension.dimension_line_point);
        translate_point(dimension.center_point);
    }
    else if (result.type == SEntityType::Hatch)
    {
        auto& hatch = std::get<SHatchEntity>(result.geometry);
        for (SPoint2d& vertex : hatch.boundary)
        {
            translate_point(vertex);
        }
        for (std::vector<SPoint2d>& island : hatch.island_boundaries)
        {
            for (SPoint2d& vertex : island)
            {
                translate_point(vertex);
            }
        }
    }
    else if (result.type == SEntityType::Spline)
    {
        transformSpline(result, translate_point);
    }
    else if (result.type == SEntityType::Ellipse)
    {
        transformEllipse(result, translate_point);
    }
    else if (result.type == SEntityType::MText)
    {
        translate_point(std::get<SMTextEntity>(result.geometry).position);
    }
    else if (result.type == SEntityType::Leader)
    {
        for (SPoint2d& point : std::get<SLeaderEntity>(result.geometry).vertices)
        {
            translate_point(point);
        }
    }
    if (result.associative_array)
    {
        if (result.associative_array->array_type == SArrayType::Path)
        {
            result.associative_array.reset();
        }
        else
        {
            SEntityRecord source_geometry;
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

SEntityRecord rotatedEntity(const SEntityRecord& source, const SPoint2d& center, double angle)
{
    SEntityRecord result = source;
    const auto rotate_point = [&center, angle](SPoint2d& point)
    {
        point = rotatePoint(point, center, angle);
    };
    if (result.type == SEntityType::Line)
    {
        auto& line = std::get<SLineEntity>(result.geometry);
        rotate_point(line.start_point);
        rotate_point(line.end_point);
    }
    else if (result.type == SEntityType::Circle)
    {
        rotate_point(std::get<SCircleEntity>(result.geometry).center);
    }
    else if (result.type == SEntityType::Arc)
    {
        auto& arc = std::get<SArcEntity>(result.geometry);
        rotate_point(arc.center);
        arc.start_angle += angle;
        arc.end_angle += angle;
    }
    else if (result.type == SEntityType::Polyline)
    {
        for (SPoint2d& vertex : std::get<SPolylineEntity>(result.geometry).vertices)
        {
            rotate_point(vertex);
        }
    }
    else if (result.type == SEntityType::Text)
    {
        auto& text = std::get<STextEntity>(result.geometry);
        rotate_point(text.position);
        text.rotation += angle;
    }
    else if (result.type == SEntityType::LinearDimension)
    {
        auto& dimension = std::get<SLinearDimensionEntity>(result.geometry);
        rotate_point(dimension.first_point);
        rotate_point(dimension.second_point);
        rotate_point(dimension.dimension_line_point);
        rotate_point(dimension.center_point);
    }
    else if (result.type == SEntityType::Hatch)
    {
        auto& hatch = std::get<SHatchEntity>(result.geometry);
        for (SPoint2d& vertex : hatch.boundary)
        {
            rotate_point(vertex);
        }
        for (std::vector<SPoint2d>& island : hatch.island_boundaries)
        {
            for (SPoint2d& vertex : island)
            {
                rotate_point(vertex);
            }
        }
    }
    else if (result.type == SEntityType::Spline)
    {
        transformSpline(result, rotate_point);
    }
    else if (result.type == SEntityType::Ellipse)
    {
        transformEllipse(result, rotate_point);
    }
    else if (result.type == SEntityType::MText)
    {
        auto& text = std::get<SMTextEntity>(result.geometry);
        rotate_point(text.position);
        text.rotation += angle;
    }
    else if (result.type == SEntityType::Leader)
    {
        for (SPoint2d& point : std::get<SLeaderEntity>(result.geometry).vertices)
        {
            rotate_point(point);
        }
    }
    return result;
}

SEntityRecord scaledEntity(const SEntityRecord& source, const SPoint2d& base_point,
                           double scale_factor)
{
    SEntityRecord result = source;
    const auto scale_point = [&base_point, scale_factor](SPoint2d& point)
    {
        point = scalePoint(point, base_point, scale_factor);
    };
    if (result.type == SEntityType::Circle)
    {
        auto& circle = std::get<SCircleEntity>(result.geometry);
        scale_point(circle.center);
        circle.radius *= std::abs(scale_factor);
    }
    else if (result.type == SEntityType::Arc)
    {
        auto& arc = std::get<SArcEntity>(result.geometry);
        scale_point(arc.center);
        arc.radius *= std::abs(scale_factor);
    }
    else if (result.type == SEntityType::Text)
    {
        auto& text = std::get<STextEntity>(result.geometry);
        scale_point(text.position);
        text.height *= std::abs(scale_factor);
    }
    else if (result.type == SEntityType::Line)
    {
        auto& line = std::get<SLineEntity>(result.geometry);
        scale_point(line.start_point);
        scale_point(line.end_point);
    }
    else if (result.type == SEntityType::Polyline)
    {
        for (SPoint2d& point : std::get<SPolylineEntity>(result.geometry).vertices)
        {
            scale_point(point);
        }
        auto& polyline = std::get<SPolylineEntity>(result.geometry);
        for (double& width : polyline.start_widths)
        {
            width *= std::abs(scale_factor);
        }
        for (double& width : polyline.end_widths)
        {
            width *= std::abs(scale_factor);
        }
    }
    else if (result.type == SEntityType::LinearDimension)
    {
        auto& dimension = std::get<SLinearDimensionEntity>(result.geometry);
        scale_point(dimension.first_point);
        scale_point(dimension.second_point);
        scale_point(dimension.dimension_line_point);
        scale_point(dimension.center_point);
    }
    else if (result.type == SEntityType::Hatch)
    {
        auto& hatch = std::get<SHatchEntity>(result.geometry);
        for (SPoint2d& point : hatch.boundary)
        {
            scale_point(point);
        }
        for (std::vector<SPoint2d>& island : hatch.island_boundaries)
        {
            for (SPoint2d& point : island)
            {
                scale_point(point);
            }
        }
    }
    else if (result.type == SEntityType::Spline)
    {
        transformSpline(result, scale_point);
    }
    else if (result.type == SEntityType::Ellipse)
    {
        transformEllipse(result, scale_point);
    }
    else if (result.type == SEntityType::MText)
    {
        auto& text = std::get<SMTextEntity>(result.geometry);
        scale_point(text.position);
        text.width *= std::abs(scale_factor);
        text.height *= std::abs(scale_factor);
    }
    else if (result.type == SEntityType::Leader)
    {
        auto& leader = std::get<SLeaderEntity>(result.geometry);
        for (SPoint2d& point : leader.vertices)
        {
            scale_point(point);
        }
        leader.text_height *= std::abs(scale_factor);
        leader.arrow_size *= std::abs(scale_factor);
    }
    return result;
}

SEntityRecord mirroredEntity(const SEntityRecord& source, const SPoint2d& axis_start,
                             const SPoint2d& axis_end)
{
    SEntityRecord result = source;
    const auto mirror_point = [&axis_start, &axis_end](SPoint2d& point)
    {
        point = mirrorPoint(point, axis_start, axis_end);
    };
    if (result.type == SEntityType::Arc)
    {
        auto& arc = std::get<SArcEntity>(result.geometry);
        const double axis_angle =
            std::atan2(axis_end.y - axis_start.y, axis_end.x - axis_start.x) * 180.0 / kPi;
        mirror_point(arc.center);
        arc.start_angle = (2.0 * axis_angle) - arc.start_angle;
        arc.end_angle = (2.0 * axis_angle) - arc.end_angle;
        arc.is_clockwise = !arc.is_clockwise;
    }
    else if (result.type == SEntityType::Text)
    {
        auto& text = std::get<STextEntity>(result.geometry);
        const double axis_angle =
            std::atan2(axis_end.y - axis_start.y, axis_end.x - axis_start.x) * 180.0 / kPi;
        mirror_point(text.position);
        text.rotation = (2.0 * axis_angle) - text.rotation;
    }
    else if (result.type == SEntityType::Polyline)
    {
        auto& polyline = std::get<SPolylineEntity>(result.geometry);
        for (SPoint2d& point : polyline.vertices)
        {
            mirror_point(point);
        }
        for (double& bulge : polyline.bulges)
        {
            bulge = -bulge;
        }
    }
    else if (result.type == SEntityType::Line)
    {
        auto& line = std::get<SLineEntity>(result.geometry);
        mirror_point(line.start_point);
        mirror_point(line.end_point);
    }
    else if (result.type == SEntityType::Circle)
    {
        mirror_point(std::get<SCircleEntity>(result.geometry).center);
    }
    else if (result.type == SEntityType::LinearDimension)
    {
        auto& dimension = std::get<SLinearDimensionEntity>(result.geometry);
        mirror_point(dimension.first_point);
        mirror_point(dimension.second_point);
        mirror_point(dimension.dimension_line_point);
        mirror_point(dimension.center_point);
    }
    else if (result.type == SEntityType::Hatch)
    {
        auto& hatch = std::get<SHatchEntity>(result.geometry);
        for (SPoint2d& point : hatch.boundary)
        {
            mirror_point(point);
        }
        for (std::vector<SPoint2d>& island : hatch.island_boundaries)
        {
            for (SPoint2d& point : island)
            {
                mirror_point(point);
            }
        }
    }
    else if (result.type == SEntityType::Spline)
    {
        transformSpline(result, mirror_point);
    }
    else if (result.type == SEntityType::Ellipse)
    {
        transformEllipse(result, mirror_point);
    }
    else if (result.type == SEntityType::MText)
    {
        auto& text = std::get<SMTextEntity>(result.geometry);
        const double axis_angle =
            std::atan2(axis_end.y - axis_start.y, axis_end.x - axis_start.x) * 180.0 / kPi;
        mirror_point(text.position);
        text.rotation = (2.0 * axis_angle) - text.rotation;
    }
    else if (result.type == SEntityType::Leader)
    {
        for (SPoint2d& point : std::get<SLeaderEntity>(result.geometry).vertices)
        {
            mirror_point(point);
        }
    }
    return result;
}

} // namespace smartGraphics
