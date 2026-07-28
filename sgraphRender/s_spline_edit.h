#pragma once

#include "s_entity.h"

#include <cstddef>

namespace smartGraphics
{

bool splineControlPointEntity(const SEntityRecord& source, std::size_t control_point_index,
                              const SPoint2d& destination, SEntityRecord& result);
bool reversedSplineEntity(const SEntityRecord& source, SEntityRecord& result);
bool splinePolylineEntity(const SEntityRecord& source, int segment_count, SEntityRecord& result);

} // namespace smartGraphics
