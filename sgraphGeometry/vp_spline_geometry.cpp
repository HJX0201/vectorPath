#include "vp_spline_geometry.h"

#include <algorithm>

namespace Vp
{

VpPoint2d splinePoint(const VpSplineEntity& spline, double parameter) noexcept
{
    const double value = std::clamp(parameter, 0.0, 1.0);
    const double inverse = 1.0 - value;
    const double first_weight = inverse * inverse * inverse;
    const double second_weight = 3.0 * inverse * inverse * value;
    const double third_weight = 3.0 * inverse * value * value;
    const double fourth_weight = value * value * value;
    return {
        (first_weight * spline.control_points[0].x) + (second_weight * spline.control_points[1].x) +
            (third_weight * spline.control_points[2].x) +
            (fourth_weight * spline.control_points[3].x),
        (first_weight * spline.control_points[0].y) + (second_weight * spline.control_points[1].y) +
            (third_weight * spline.control_points[2].y) +
            (fourth_weight * spline.control_points[3].y)};
}

VpPoint2d splineTangent(const VpSplineEntity& spline, double parameter) noexcept
{
    const double value = std::clamp(parameter, 0.0, 1.0);
    const double inverse = 1.0 - value;
    return {3.0 * inverse * inverse * (spline.control_points[1].x - spline.control_points[0].x) +
                6.0 * inverse * value * (spline.control_points[2].x - spline.control_points[1].x) +
                3.0 * value * value * (spline.control_points[3].x - spline.control_points[2].x),
            3.0 * inverse * inverse * (spline.control_points[1].y - spline.control_points[0].y) +
                6.0 * inverse * value * (spline.control_points[2].y - spline.control_points[1].y) +
                3.0 * value * value * (spline.control_points[3].y - spline.control_points[2].y)};
}

std::vector<VpPoint2d> splineApproximation(const VpSplineEntity& spline, int segment_count)
{
    const int safe_segment_count = std::clamp(segment_count, 4, 4096);
    std::vector<VpPoint2d> points;
    points.reserve(static_cast<std::size_t>(safe_segment_count + 1));
    for (int index = 0; index <= safe_segment_count; ++index)
    {
        points.push_back(splinePoint(spline, static_cast<double>(index) / safe_segment_count));
    }
    return points;
}

double splineApproximateLength(const VpSplineEntity& spline, int segment_count)
{
    const std::vector<VpPoint2d> points = splineApproximation(spline, segment_count);
    double result = 0.0;
    for (std::size_t index = 1; index < points.size(); ++index)
    {
        result += distance(points[index - 1], points[index]);
    }
    return result;
}

} // namespace Vp
