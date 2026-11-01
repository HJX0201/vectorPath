#pragma once

#include "vp_curve_entities.h"

#include <vector>

namespace Vp
{

// 按参数计算样条上的点。
VpPoint2d splinePoint(const VpSplineEntity& spline, double parameter) noexcept;
// 按参数计算样条的切向。
VpPoint2d splineTangent(const VpSplineEntity& spline, double parameter) noexcept;
// 用折线段逼近样条。
std::vector<VpPoint2d> splineApproximation(const VpSplineEntity& spline, int segment_count = 48);
// 估算样条弧长。
double splineApproximateLength(const VpSplineEntity& spline, int segment_count = 96);

} // namespace Vp
