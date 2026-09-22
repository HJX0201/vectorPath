#pragma once

#include "vp_geometry_types.h"

#include <vector>

namespace Vp
{

bool pointInsidePolygon(const VpPoint2d& point, const std::vector<VpPoint2d>& polygon) noexcept;
double hatchLoopArea(const std::vector<VpPoint2d>& loop) noexcept;

} // namespace Vp
