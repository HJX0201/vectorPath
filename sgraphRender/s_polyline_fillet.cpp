#include "s_cad_viewport_geometry.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace smartGraphics
{

bool filletedPolylineEntity(const SEntityRecord& source, double radius, SEntityRecord& result)
{
    if (source.type != SEntityType::Polyline || radius <= 1.0e-9 || !std::isfinite(radius))
    {
        return false;
    }
    const auto& polyline = std::get<SPolylineEntity>(source.geometry);
    const std::size_t vertex_count = polyline.vertices.size();
    const bool has_arc_segments = std::any_of(polyline.bulges.begin(), polyline.bulges.end(),
                                              [](double bulge)
                                              {
                                                  return std::abs(bulge) > 1.0e-12;
                                              });
    if (vertex_count < 3 || has_arc_segments)
    {
        return false;
    }
    std::vector<double> tangent_distances(vertex_count, 0.0);
    std::vector<double> corner_bulges(vertex_count, 0.0);
    const std::size_t first_corner = polyline.is_closed ? 0U : 1U;
    const std::size_t corner_end = polyline.is_closed ? vertex_count : vertex_count - 1;
    for (std::size_t index = first_corner; index < corner_end; ++index)
    {
        const SPoint2d& previous = polyline.vertices[(index + vertex_count - 1) % vertex_count];
        const SPoint2d& corner = polyline.vertices[index];
        const SPoint2d& next = polyline.vertices[(index + 1) % vertex_count];
        const double incoming_length = distance(previous, corner);
        const double outgoing_length = distance(corner, next);
        if (incoming_length <= 1.0e-9 || outgoing_length <= 1.0e-9)
        {
            return false;
        }
        const SPoint2d ray_to_previous{(previous.x - corner.x) / incoming_length,
                                       (previous.y - corner.y) / incoming_length};
        const SPoint2d ray_to_next{(next.x - corner.x) / outgoing_length,
                                   (next.y - corner.y) / outgoing_length};
        const double direction_dot = std::clamp(
            (ray_to_previous.x * ray_to_next.x) + (ray_to_previous.y * ray_to_next.y), -1.0, 1.0);
        const double included_angle = std::acos(direction_dot);
        if (included_angle <= 1.0e-6)
        {
            return false;
        }
        const double turn_angle = 3.14159265358979323846 - included_angle;
        if (turn_angle <= 1.0e-6)
        {
            continue;
        }
        const double tangent_distance = radius / std::tan(included_angle * 0.5);
        if (!std::isfinite(tangent_distance))
        {
            return false;
        }
        const double incoming_x = corner.x - previous.x;
        const double incoming_y = corner.y - previous.y;
        const double outgoing_x = next.x - corner.x;
        const double outgoing_y = next.y - corner.y;
        const double turn_cross = (incoming_x * outgoing_y) - (incoming_y * outgoing_x);
        tangent_distances[index] = tangent_distance;
        corner_bulges[index] = std::copysign(std::tan(turn_angle * 0.25), turn_cross);
    }
    const std::size_t segment_count = polyline.is_closed ? vertex_count : vertex_count - 1;
    for (std::size_t index = 0; index < segment_count; ++index)
    {
        const std::size_t end_index = (index + 1) % vertex_count;
        if (distance(polyline.vertices[index], polyline.vertices[end_index]) <=
            tangent_distances[index] + tangent_distances[end_index] + 1.0e-9)
        {
            return false;
        }
    }
    std::vector<SPoint2d> vertices;
    std::vector<double> bulges;
    const auto append = [&](const SPoint2d& point, double bulge)
    {
        vertices.push_back(point);
        bulges.push_back(bulge);
    };
    if (!polyline.is_closed)
    {
        append(polyline.vertices.front(), 0.0);
    }
    for (std::size_t index = first_corner; index < corner_end; ++index)
    {
        const SPoint2d& previous = polyline.vertices[(index + vertex_count - 1) % vertex_count];
        const SPoint2d& corner = polyline.vertices[index];
        const SPoint2d& next = polyline.vertices[(index + 1) % vertex_count];
        const double tangent_distance = tangent_distances[index];
        if (tangent_distance <= 1.0e-9)
        {
            append(corner, 0.0);
            continue;
        }
        const double incoming_length = distance(previous, corner);
        const double outgoing_length = distance(corner, next);
        append({corner.x + ((previous.x - corner.x) * tangent_distance / incoming_length),
                corner.y + ((previous.y - corner.y) * tangent_distance / incoming_length)},
               corner_bulges[index]);
        append({corner.x + ((next.x - corner.x) * tangent_distance / outgoing_length),
                corner.y + ((next.y - corner.y) * tangent_distance / outgoing_length)},
               0.0);
    }
    if (!polyline.is_closed)
    {
        append(polyline.vertices.back(), 0.0);
    }
    result = source;
    result.geometry = SPolylineEntity{std::move(vertices), polyline.is_closed, std::move(bulges)};
    return true;
}

} // namespace smartGraphics
