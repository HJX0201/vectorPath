#include "s_geometry_types.h"

namespace vectorPath
{

double distance(const SPoint2d& first_point, const SPoint2d& second_point) noexcept
{
    const double delta_x = second_point.x - first_point.x;
    const double delta_y = second_point.y - first_point.y;
    return std::sqrt((delta_x * delta_x) + (delta_y * delta_y));
}

QString formatPoint(const SPoint2d& point, int precision)
{
    return QStringLiteral("%1, %2").arg(point.x, 0, 'f', precision).arg(point.y, 0, 'f', precision);
}

} // namespace vectorPath
