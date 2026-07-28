#pragma once

#include "s_entity.h"

#include <vector>

namespace smartGraphics
{

SPoint2d splinePoint(const SSplineEntity& spline, double parameter) noexcept;
SPoint2d splineTangent(const SSplineEntity& spline, double parameter) noexcept;
std::vector<SPoint2d> splineApproximation(const SSplineEntity& spline, int segment_count = 48);
double splineApproximateLength(const SSplineEntity& spline, int segment_count = 96);

} // namespace smartGraphics
