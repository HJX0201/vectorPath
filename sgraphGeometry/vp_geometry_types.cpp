#include "vp_geometry_types.h"

namespace Vp
{

double distance(const VpPoint2d& first_point, const VpPoint2d& second_point) noexcept
{
    const double delta_x = second_point.x - first_point.x;
    const double delta_y = second_point.y - first_point.y;
    return std::sqrt((delta_x * delta_x) + (delta_y * delta_y));
}

} // namespace Vp
