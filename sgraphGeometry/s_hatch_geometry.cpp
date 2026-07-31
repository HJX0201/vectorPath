#include "s_hatch_geometry.h"

#include <algorithm>
#include <cmath>

namespace smartCam
{

bool pointInsidePolygon(const SPoint2d& point, const std::vector<SPoint2d>& polygon) noexcept
{
    if (polygon.size() < 3)
    {
        return false;
    }
    bool is_inside = false;
    for (std::size_t index = 0, previous = polygon.size() - 1; index < polygon.size();
         previous = index++)
    {
        const SPoint2d& first = polygon[index];
        const SPoint2d& second = polygon[previous];
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

double hatchLoopArea(const std::vector<SPoint2d>& loop) noexcept
{
    if (loop.size() < 3)
    {
        return 0.0;
    }
    double twice_area = 0.0;
    for (std::size_t index = 0; index < loop.size(); ++index)
    {
        const SPoint2d& current = loop[index];
        const SPoint2d& next = loop[(index + 1) % loop.size()];
        twice_area += (current.x * next.y) - (next.x * current.y);
    }
    return std::abs(twice_area) * 0.5;
}

bool isHatchValid(const SHatchEntity& hatch) noexcept
{
    const auto valid_loop = [](const std::vector<SPoint2d>& loop)
    {
        return loop.size() >= 3 && hatchLoopArea(loop) > 1.0e-12 &&
               std::all_of(loop.begin(), loop.end(),
                           [](const SPoint2d& point)
                           {
                               return std::isfinite(point.x) && std::isfinite(point.y);
                           });
    };
    if (!valid_loop(hatch.boundary) ||
        !std::all_of(hatch.island_boundaries.begin(), hatch.island_boundaries.end(), valid_loop) ||
        !std::isfinite(hatch.pattern_scale) || hatch.pattern_scale <= 0.0 ||
        !std::isfinite(hatch.pattern_angle) || hatch.pattern_name.trimmed().isEmpty() ||
        !hatch.gradient_start.isValid() || !hatch.gradient_end.isValid())
    {
        return false;
    }
    return std::all_of(hatch.island_boundaries.begin(), hatch.island_boundaries.end(),
                       [&hatch](const std::vector<SPoint2d>& island)
                       {
                           return pointInsidePolygon(island.front(), hatch.boundary);
                       });
}

} // namespace smartCam
