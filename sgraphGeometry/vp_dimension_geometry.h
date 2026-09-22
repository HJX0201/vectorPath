#pragma once

#include "vp_entity.h"

#include <QString>
#include <vector>

namespace Vp
{

double dimensionMeasurement(const VpLinearDimensionEntity& dimension) noexcept;
QString dimensionTypeName(VpDimensionType dimension_type);
QString dimensionDefaultText(const VpLinearDimensionEntity& dimension, int linear_precision = 2,
                             int angular_precision = 2);
std::vector<VpPoint2d> dimensionReferencePoints(const VpLinearDimensionEntity& dimension);
bool isDimensionValid(const VpLinearDimensionEntity& dimension) noexcept;

} // namespace Vp
