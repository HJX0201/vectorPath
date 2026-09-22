#include "vp_standard_shape.h"

#include <cmath>

namespace Vp
{
namespace
{

constexpr double kPi = 3.14159265358979323846;

int standardShapeSideCount(VpStandardShapeType shape_type)
{
    switch (shape_type)
    {
    case VpStandardShapeType::Triangle:
        return 3;
    case VpStandardShapeType::Pentagon:
        return 5;
    case VpStandardShapeType::Hexagon:
        return 6;
    case VpStandardShapeType::Octagon:
        return 8;
    case VpStandardShapeType::Diamond:
        return 4;
    case VpStandardShapeType::FivePointStar:
        return 10;
    }
    return 0;
}

} // namespace

std::vector<VpPoint2d> standardShapeVertices(VpStandardShapeType shape_type,
                                             const VpPoint2d& center, const VpPoint2d& radius_point)
{
    const double delta_x = radius_point.x - center.x;
    const double delta_y = radius_point.y - center.y;
    const double radius = std::hypot(delta_x, delta_y);
    if (radius <= 1.0e-9)
    {
        return {};
    }

    const int vertex_count = standardShapeSideCount(shape_type);
    const double start_angle = std::atan2(delta_y, delta_x);
    std::vector<VpPoint2d> vertices;
    vertices.reserve(static_cast<std::size_t>(vertex_count));
    for (int index = 0; index < vertex_count; ++index)
    {
        double vertex_radius = radius;
        if (shape_type == VpStandardShapeType::FivePointStar && index % 2 != 0)
        {
            vertex_radius *= 0.3819660112501051;
        }
        const double angle = start_angle + 2.0 * kPi * static_cast<double>(index) / vertex_count;
        vertices.push_back({center.x + std::cos(angle) * vertex_radius,
                            center.y + std::sin(angle) * vertex_radius});
    }
    return vertices;
}

} // namespace Vp
