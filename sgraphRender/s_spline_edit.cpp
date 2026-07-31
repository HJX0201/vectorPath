#include "s_spline_edit.h"

#include "s_spline_geometry.h"

#include <algorithm>
#include <cmath>

namespace vectorPath
{

bool splineControlPointEntity(const SEntityRecord& source, std::size_t control_point_index,
                              const SPoint2d& destination, SEntityRecord& result)
{
    if (source.type != SEntityType::Spline || control_point_index >= 4 ||
        !std::isfinite(destination.x) || !std::isfinite(destination.y))
    {
        return false;
    }
    result = source;
    auto& spline = std::get<SSplineEntity>(result.geometry);
    if (distance(spline.control_points[control_point_index], destination) <= 1.0e-12)
    {
        return false;
    }
    spline.control_points[control_point_index] = destination;
    result.associative_array.reset();
    return true;
}

bool reversedSplineEntity(const SEntityRecord& source, SEntityRecord& result)
{
    if (source.type != SEntityType::Spline)
    {
        return false;
    }
    result = source;
    auto& control_points = std::get<SSplineEntity>(result.geometry).control_points;
    std::reverse(control_points.begin(), control_points.end());
    result.associative_array.reset();
    return true;
}

bool splinePolylineEntity(const SEntityRecord& source, int segment_count, SEntityRecord& result)
{
    if (source.type != SEntityType::Spline || segment_count < 4 || segment_count > 4096)
    {
        return false;
    }
    result = source;
    result.type = SEntityType::Polyline;
    result.geometry = SPolylineEntity{
        splineApproximation(std::get<SSplineEntity>(source.geometry), segment_count), false};
    result.associative_array.reset();
    return true;
}

} // namespace vectorPath
