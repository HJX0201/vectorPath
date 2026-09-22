#include "vp_ellipse_geometry.h"

#include <algorithm>
#include <cmath>

namespace Vp
{
namespace
{

constexpr double kPi = 3.14159265358979323846;

} // namespace

bool isValidEllipse(const VpEllipseEntity& ellipse) noexcept
{
    return std::hypot(ellipse.major_axis.x, ellipse.major_axis.y) > 1.0e-9 &&
           std::hypot(ellipse.minor_axis.x, ellipse.minor_axis.y) > 1.0e-9;
}

VpPoint2d ellipsePoint(const VpEllipseEntity& ellipse, double parameter) noexcept
{
    return {ellipse.center.x + (ellipse.major_axis.x * std::cos(parameter)) +
                (ellipse.minor_axis.x * std::sin(parameter)),
            ellipse.center.y + (ellipse.major_axis.y * std::cos(parameter)) +
                (ellipse.minor_axis.y * std::sin(parameter))};
}

std::vector<VpPoint2d> ellipseApproximation(const VpEllipseEntity& ellipse, int segment_count)
{
    std::vector<VpPoint2d> points;
    if (!isValidEllipse(ellipse))
    {
        return points;
    }
    segment_count = std::clamp(segment_count, 12, 4096);
    points.reserve(static_cast<std::size_t>(segment_count + 1));
    for (int index = 0; index <= segment_count; ++index)
    {
        points.push_back(ellipsePoint(ellipse, (2.0 * kPi * index) / segment_count));
    }
    return points;
}

} // namespace Vp
