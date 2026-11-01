#pragma once

#include "vp_curve_entities.h"

#include <vector>

namespace Vp
{

// 判断椭圆定义是否有效。
bool isValidEllipse(const VpEllipseEntity& ellipse) noexcept;
// 按参数角计算椭圆上的点。
VpPoint2d ellipsePoint(const VpEllipseEntity& ellipse, double parameter) noexcept;
// 用折线段逼近椭圆。
std::vector<VpPoint2d> ellipseApproximation(const VpEllipseEntity& ellipse, int segment_count = 96);

} // namespace Vp
