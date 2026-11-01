#pragma once

#include "vp_entity.h"
#include "vp_polygon_geometry.h"

namespace Vp
{

// 判断填充边界是否有效。
bool isHatchValid(const VpHatchEntity& hatch) noexcept;

} // namespace Vp
