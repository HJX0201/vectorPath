#include "s_cad_viewport_geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace vectorPath
{

bool bulgeArc(const SPoint2d& start_point, const SPoint2d& end_point, double bulge,
              SArcEntity& result) noexcept
{
    const double chord_x = end_point.x - start_point.x;
    const double chord_y = end_point.y - start_point.y;
    const double chord_length = std::hypot(chord_x, chord_y);
    if (chord_length <= 1.0e-12 || std::abs(bulge) <= 1.0e-12 || !std::isfinite(bulge))
    {
        return false;
    }
    const SPoint2d midpoint{(start_point.x + end_point.x) * 0.5,
                            (start_point.y + end_point.y) * 0.5};
    const double center_offset = chord_length * (1.0 - (bulge * bulge)) / (4.0 * bulge);
    const SPoint2d center{midpoint.x - ((chord_y / chord_length) * center_offset),
                          midpoint.y + ((chord_x / chord_length) * center_offset)};
    const double radius = chord_length * (1.0 + (bulge * bulge)) / (4.0 * std::abs(bulge));
    double start_angle = entityAngleDegrees(center, start_point);
    double end_angle = entityAngleDegrees(center, end_point);
    if (bulge < 0.0)
    {
        std::swap(start_angle, end_angle);
    }
    result = SArcEntity{center, radius, start_angle, end_angle};
    return true;
}

bool threePointBulge(const SPoint2d& start_point, const SPoint2d& point_on_arc,
                     const SPoint2d& end_point, double& bulge) noexcept
{
    SPoint2d center;
    double radius = 0.0;
    double ignored_start = 0.0;
    double ignored_end = 0.0;
    if (!calculateThreePointArc(start_point, point_on_arc, end_point, center, radius, ignored_start,
                                ignored_end))
    {
        return false;
    }
    const double start_angle = entityAngleDegrees(center, start_point);
    const double middle_angle = entityAngleDegrees(center, point_on_arc);
    const double end_angle = entityAngleDegrees(center, end_point);
    const double counter_clockwise_span = std::fmod(end_angle - start_angle + 360.0, 360.0);
    const double middle_span = std::fmod(middle_angle - start_angle + 360.0, 360.0);
    const double signed_span = middle_span <= counter_clockwise_span + 1.0e-9
                                   ? counter_clockwise_span
                                   : -(360.0 - counter_clockwise_span);
    bulge = std::tan(signed_span * 3.14159265358979323846 / 720.0);
    return std::isfinite(bulge) && std::abs(bulge) > 1.0e-12;
}

std::vector<SPoint2d> polylineSegmentOutline(const SPolylineEntity& polyline,
                                             std::size_t segment_index, int arc_segment_count)
{
    const std::size_t vertex_count = polyline.vertices.size();
    const std::size_t segment_count =
        vertex_count < 2 ? 0 : (polyline.is_closed ? vertex_count : vertex_count - 1);
    if (segment_index >= segment_count)
    {
        return {};
    }
    const double start_width =
        segment_index < polyline.start_widths.size() ? polyline.start_widths[segment_index] : 0.0;
    const double end_width =
        segment_index < polyline.end_widths.size() ? polyline.end_widths[segment_index] : 0.0;
    if (start_width <= 1.0e-12 && end_width <= 1.0e-12)
    {
        return {};
    }
    const SPoint2d& start_point = polyline.vertices[segment_index];
    const SPoint2d& end_point = polyline.vertices[(segment_index + 1) % vertex_count];
    std::vector<SPoint2d> center_points;
    const double bulge =
        segment_index < polyline.bulges.size() ? polyline.bulges[segment_index] : 0.0;
    SArcEntity arc;
    if (bulgeArc(start_point, end_point, bulge, arc))
    {
        const int sample_count = std::clamp(arc_segment_count, 4, 512);
        const double start_angle = entityAngleDegrees(arc.center, start_point);
        const double signed_span = 4.0 * std::atan(bulge);
        center_points.reserve(static_cast<std::size_t>(sample_count + 1));
        for (int index = 0; index <= sample_count; ++index)
        {
            const double parameter = static_cast<double>(index) / sample_count;
            const double angle =
                start_angle * 3.14159265358979323846 / 180.0 + (signed_span * parameter);
            center_points.push_back({arc.center.x + (arc.radius * std::cos(angle)),
                                     arc.center.y + (arc.radius * std::sin(angle))});
        }
    }
    else
    {
        center_points = {start_point, end_point};
    }
    std::vector<SPoint2d> left_points;
    std::vector<SPoint2d> right_points;
    left_points.reserve(center_points.size());
    right_points.reserve(center_points.size());
    for (std::size_t index = 0; index < center_points.size(); ++index)
    {
        const SPoint2d& previous = center_points[index == 0 ? 0 : index - 1];
        const SPoint2d& next = center_points[index + 1 < center_points.size() ? index + 1 : index];
        const double tangent_x = next.x - previous.x;
        const double tangent_y = next.y - previous.y;
        const double tangent_length = std::hypot(tangent_x, tangent_y);
        if (tangent_length <= 1.0e-12)
        {
            return {};
        }
        const double parameter = static_cast<double>(index) / (center_points.size() - 1);
        const double half_width = (start_width + ((end_width - start_width) * parameter)) * 0.5;
        const double normal_x = -tangent_y / tangent_length;
        const double normal_y = tangent_x / tangent_length;
        left_points.push_back({center_points[index].x + (normal_x * half_width),
                               center_points[index].y + (normal_y * half_width)});
        right_points.push_back({center_points[index].x - (normal_x * half_width),
                                center_points[index].y - (normal_y * half_width)});
    }
    std::vector<SPoint2d> outline = std::move(left_points);
    outline.insert(outline.end(), right_points.rbegin(), right_points.rend());
    return outline;
}

} // namespace vectorPath
