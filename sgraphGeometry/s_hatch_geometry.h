#pragma once

#include "s_entity.h"

namespace vectorPath
{

bool pointInsidePolygon(const SPoint2d& point, const std::vector<SPoint2d>& polygon) noexcept;
double hatchLoopArea(const std::vector<SPoint2d>& loop) noexcept;
bool isHatchValid(const SHatchEntity& hatch) noexcept;

} // namespace vectorPath
