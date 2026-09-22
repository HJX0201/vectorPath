#pragma once

#include "vp_entity.h"

#include <cstddef>

namespace Vp
{

bool splineControlPointEntity(const VpEntityRecord& source, std::size_t control_point_index,
                              const VpPoint2d& destination, VpEntityRecord& result);
bool reversedSplineEntity(const VpEntityRecord& source, VpEntityRecord& result);
bool splinePolylineEntity(const VpEntityRecord& source, int segment_count, VpEntityRecord& result);

} // namespace Vp
