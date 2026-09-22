#pragma once

#include "vp_curve_entities.h"

#include <vector>

namespace Vp
{

bool isValidEllipse(const VpEllipseEntity& ellipse) noexcept;
VpPoint2d ellipsePoint(const VpEllipseEntity& ellipse, double parameter) noexcept;
std::vector<VpPoint2d> ellipseApproximation(const VpEllipseEntity& ellipse, int segment_count = 96);

} // namespace Vp
