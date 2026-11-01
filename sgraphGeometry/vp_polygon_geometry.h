#pragma once

#include "vp_geometry_types.h"

#include <vector>

namespace Vp
{

// 判断点是否在多边形内部。
bool pointInsidePolygon(const VpPoint2d& point, const std::vector<VpPoint2d>& polygon) noexcept;
// 计算填充环的有向面积。
double hatchLoopArea(const std::vector<VpPoint2d>& loop) noexcept;

} // namespace Vp
