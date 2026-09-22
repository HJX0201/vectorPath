#include "vp_hatch_geometry.h"

#include <algorithm>
#include <cmath>

namespace Vp
{

bool isHatchValid(const VpHatchEntity& hatch) noexcept
{
    const auto valid_loop = [](const std::vector<VpPoint2d>& loop)
    {
        return loop.size() >= 3 && hatchLoopArea(loop) > 1.0e-12 &&
               std::all_of(loop.begin(), loop.end(),
                           [](const VpPoint2d& point)
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
                       [&hatch](const std::vector<VpPoint2d>& island)
                       {
                           return pointInsidePolygon(island.front(), hatch.boundary);
                       });
}

} // namespace Vp
