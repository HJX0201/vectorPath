#include "vp_spline_edit.h"

#include "vp_spline_geometry.h"

#include <algorithm>
#include <cmath>

namespace Vp
{

bool splineControlPointEntity(const VpEntityRecord& source, std::size_t control_point_index,
                              const VpPoint2d& destination, VpEntityRecord& result)
{
    if (source.type != VpEntityType::Spline || control_point_index >= 4 ||
        !std::isfinite(destination.x) || !std::isfinite(destination.y))
    {
        return false;
    }
    result = source;
    auto& spline = std::get<VpSplineEntity>(result.geometry);
    if (distance(spline.control_points[control_point_index], destination) <= 1.0e-12)
    {
        return false;
    }
    spline.control_points[control_point_index] = destination;
    result.associative_array.reset();
    return true;
}

bool reversedSplineEntity(const VpEntityRecord& source, VpEntityRecord& result)
{
    if (source.type != VpEntityType::Spline)
    {
        return false;
    }
    result = source;
    auto& control_points = std::get<VpSplineEntity>(result.geometry).control_points;
    std::reverse(control_points.begin(), control_points.end());
    result.associative_array.reset();
    return true;
}

bool splinePolylineEntity(const VpEntityRecord& source, int segment_count, VpEntityRecord& result)
{
    if (source.type != VpEntityType::Spline || segment_count < 4 || segment_count > 4096)
    {
        return false;
    }
    result = source;
    result.type = VpEntityType::Polyline;
    result.geometry = VpPolylineEntity{
        splineApproximation(std::get<VpSplineEntity>(source.geometry), segment_count), false};
    result.associative_array.reset();
    return true;
}

} // namespace Vp
