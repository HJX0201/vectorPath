#pragma once

#include "s_entity.h"

#include <QString>
#include <vector>

namespace smartCam
{

double dimensionMeasurement(const SLinearDimensionEntity& dimension) noexcept;
QString dimensionTypeName(SDimensionType dimension_type);
QString dimensionDefaultText(const SLinearDimensionEntity& dimension, int linear_precision = 2,
                             int angular_precision = 2);
std::vector<SPoint2d> dimensionReferencePoints(const SLinearDimensionEntity& dimension);
bool isDimensionValid(const SLinearDimensionEntity& dimension) noexcept;

} // namespace smartCam
