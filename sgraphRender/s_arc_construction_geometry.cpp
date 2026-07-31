#include "s_cad_viewport_geometry.h"

#include <algorithm>
#include <cmath>

namespace smartCam
{
namespace
{

constexpr double kPi = 3.14159265358979323846;
constexpr double kEpsilon = 1.0e-9;

SPoint2d midpoint(const SPoint2d& first, const SPoint2d& second)
{
    return {(first.x + second.x) * 0.5, (first.y + second.y) * 0.5};
}

double crossProduct(const SPoint2d& first, const SPoint2d& second)
{
    return (first.x * second.y) - (first.y * second.x);
}

bool finitePoint(const SPoint2d& point)
{
    return std::isfinite(point.x) && std::isfinite(point.y);
}

} // namespace

bool arcFromStartEndRadius(const SPoint2d& start_point, const SPoint2d& end_point, double radius,
                           double side_sign, SArcEntity& result) noexcept
{
    const SPoint2d chord{end_point.x - start_point.x, end_point.y - start_point.y};
    const double chord_length = std::hypot(chord.x, chord.y);
    if (!finitePoint(start_point) || !finitePoint(end_point) || !std::isfinite(radius) ||
        chord_length <= kEpsilon || radius < (chord_length * 0.5) - kEpsilon)
    {
        return false;
    }
    radius = std::max(radius, chord_length * 0.5);
    const SPoint2d center_point = midpoint(start_point, end_point);
    const double center_offset =
        std::sqrt(std::max(0.0, (radius * radius) - (chord_length * chord_length * 0.25)));
    const double normalized_side = side_sign < 0.0 ? -1.0 : 1.0;
    const SPoint2d center{
        center_point.x + (normalized_side * -chord.y * center_offset / chord_length),
        center_point.y + (normalized_side * chord.x * center_offset / chord_length)};
    const double start_angle = entityAngleDegrees(center, start_point);
    const double end_angle = entityAngleDegrees(center, end_point);
    result = normalized_side > 0.0 ? SArcEntity{center, radius, start_angle, end_angle}
                                   : SArcEntity{center, radius, end_angle, start_angle};
    return true;
}

bool arcFromStartEndDirection(const SPoint2d& start_point, const SPoint2d& end_point,
                              double direction_angle, SArcEntity& result) noexcept
{
    if (!finitePoint(start_point) || !finitePoint(end_point) || !std::isfinite(direction_angle))
    {
        return false;
    }
    const SPoint2d chord{end_point.x - start_point.x, end_point.y - start_point.y};
    const double chord_squared = (chord.x * chord.x) + (chord.y * chord.y);
    if (chord_squared <= kEpsilon * kEpsilon)
    {
        return false;
    }
    const double direction_radians = direction_angle * kPi / 180.0;
    const SPoint2d left_normal{-std::sin(direction_radians), std::cos(direction_radians)};
    const double denominator = 2.0 * ((chord.x * left_normal.x) + (chord.y * left_normal.y));
    if (std::abs(denominator) <= kEpsilon * std::sqrt(chord_squared))
    {
        return false;
    }
    const double signed_radius = chord_squared / denominator;
    const SPoint2d center{start_point.x + (left_normal.x * signed_radius),
                          start_point.y + (left_normal.y * signed_radius)};
    const double radius = std::abs(signed_radius);
    const double start_angle = entityAngleDegrees(center, start_point);
    const double end_angle = entityAngleDegrees(center, end_point);
    result = signed_radius > 0.0 ? SArcEntity{center, radius, start_angle, end_angle}
                                 : SArcEntity{center, radius, end_angle, start_angle};
    return std::isfinite(radius) && radius > kEpsilon;
}

bool arcFromStartEndAngle(const SPoint2d& start_point, const SPoint2d& end_point,
                          double included_angle, SArcEntity& result) noexcept
{
    const SPoint2d chord{end_point.x - start_point.x, end_point.y - start_point.y};
    const double chord_length = std::hypot(chord.x, chord.y);
    const double half_angle_radians = included_angle * kPi / 360.0;
    if (!finitePoint(start_point) || !finitePoint(end_point) || !std::isfinite(included_angle) ||
        chord_length <= kEpsilon || std::abs(included_angle) <= 1.0e-7 ||
        std::abs(included_angle) >= 360.0 - 1.0e-7 ||
        std::abs(std::sin(half_angle_radians)) <= kEpsilon)
    {
        return false;
    }
    const double center_offset = chord_length / (2.0 * std::tan(half_angle_radians));
    const SPoint2d center_point = midpoint(start_point, end_point);
    const SPoint2d center{center_point.x - (chord.y * center_offset / chord_length),
                          center_point.y + (chord.x * center_offset / chord_length)};
    const double radius = chord_length / (2.0 * std::abs(std::sin(half_angle_radians)));
    const double start_angle = entityAngleDegrees(center, start_point);
    const double end_angle = entityAngleDegrees(center, end_point);
    result = included_angle > 0.0 ? SArcEntity{center, radius, start_angle, end_angle}
                                  : SArcEntity{center, radius, end_angle, start_angle};
    return std::isfinite(radius) && radius > kEpsilon;
}

bool arcFromCenterStartAngle(const SPoint2d& center, const SPoint2d& start_point,
                             double included_angle, SArcEntity& result) noexcept
{
    const double radius = distance(center, start_point);
    if (!finitePoint(center) || !finitePoint(start_point) || !std::isfinite(included_angle) ||
        radius <= kEpsilon || std::abs(included_angle) <= 1.0e-7 ||
        std::abs(included_angle) >= 360.0 - 1.0e-7)
    {
        return false;
    }
    const double start_angle = entityAngleDegrees(center, start_point);
    const double end_angle = start_angle + included_angle;
    result = included_angle > 0.0 ? SArcEntity{center, radius, start_angle, end_angle}
                                  : SArcEntity{center, radius, end_angle, start_angle};
    return true;
}

} // namespace smartCam
