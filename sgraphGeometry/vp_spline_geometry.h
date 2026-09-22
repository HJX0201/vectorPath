#pragma once

#include "vp_curve_entities.h"

#include <vector>

namespace Vp
{

VpPoint2d splinePoint(const VpSplineEntity& spline, double parameter) noexcept;
VpPoint2d splineTangent(const VpSplineEntity& spline, double parameter) noexcept;
std::vector<VpPoint2d> splineApproximation(const VpSplineEntity& spline, int segment_count = 48);
double splineApproximateLength(const VpSplineEntity& spline, int segment_count = 96);

} // namespace Vp
