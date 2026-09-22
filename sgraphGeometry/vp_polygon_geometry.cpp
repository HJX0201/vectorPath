#include "vp_polygon_geometry.h"

#include <cmath>

namespace Vp
{

bool pointInsidePolygon(const VpPoint2d& point, const std::vector<VpPoint2d>& polygon) noexcept
{
    if (polygon.size() < 3)
    {
        return false;
    }
    bool is_inside = false;
    for (std::size_t index = 0, previous = polygon.size() - 1; index < polygon.size();
         previous = index++)
    {
        const VpPoint2d& first = polygon[index];
        const VpPoint2d& second = polygon[previous];
        const bool crosses = ((first.y > point.y) != (second.y > point.y)) &&
                             (point.x < (second.x - first.x) * (point.y - first.y) /
                                                ((second.y - first.y) + 1.0e-30) +
                                            first.x);
        if (crosses)
        {
            is_inside = !is_inside;
        }
    }
    return is_inside;
}

double hatchLoopArea(const std::vector<VpPoint2d>& loop) noexcept
{
    if (loop.size() < 3)
    {
        return 0.0;
    }
    double twice_area = 0.0;
    for (std::size_t index = 0; index < loop.size(); ++index)
    {
        const VpPoint2d& current = loop[index];
        const VpPoint2d& next = loop[(index + 1) % loop.size()];
        twice_area += (current.x * next.y) - (next.x * current.y);
    }
    return std::abs(twice_area) * 0.5;
}

} // namespace Vp
