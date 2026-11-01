#pragma once

#include "vp_entity.h"

#include <cstddef>

namespace Vp
{

// 移动样条控制点并重建样条。
bool splineControlPointEntity(const VpEntityRecord& source, std::size_t control_point_index,
                              const VpPoint2d& destination, VpEntityRecord& result);
// 反转样条方向。
bool reversedSplineEntity(const VpEntityRecord& source, VpEntityRecord& result);
// 把样条离散为多段线。
bool splinePolylineEntity(const VpEntityRecord& source, int segment_count, VpEntityRecord& result);

} // namespace Vp
