#include "s_ellipse_geometry.h"

#include <algorithm>
#include <cmath>

namespace smartGraphics
{
namespace
{

constexpr double kPi = 3.14159265358979323846;

} // namespace

bool isValidEllipse(const SEllipseEntity& ellipse) noexcept
{
    return std::hypot(ellipse.major_axis.x, ellipse.major_axis.y) > 1.0e-9 &&
           std::hypot(ellipse.minor_axis.x, ellipse.minor_axis.y) > 1.0e-9;
}

SPoint2d ellipsePoint(const SEllipseEntity& ellipse, double parameter) noexcept
{
    return {ellipse.center.x + (ellipse.major_axis.x * std::cos(parameter)) +
                (ellipse.minor_axis.x * std::sin(parameter)),
            ellipse.center.y + (ellipse.major_axis.y * std::cos(parameter)) +
                (ellipse.minor_axis.y * std::sin(parameter))};
}

std::vector<SPoint2d> ellipseApproximation(const SEllipseEntity& ellipse, int segment_count)
{
    std::vector<SPoint2d> points;
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

} // namespace smartGraphics
