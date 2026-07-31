#pragma once

#include "s_entity.h"

#include <vector>

namespace vectorPath
{

bool isValidEllipse(const SEllipseEntity& ellipse) noexcept;
SPoint2d ellipsePoint(const SEllipseEntity& ellipse, double parameter) noexcept;
std::vector<SPoint2d> ellipseApproximation(const SEllipseEntity& ellipse, int segment_count = 96);

} // namespace vectorPath
