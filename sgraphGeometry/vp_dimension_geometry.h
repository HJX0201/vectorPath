#pragma once

#include "vp_entity.h"

#include <QString>
#include <vector>

namespace Vp
{

// 计算标注的测量值。
double dimensionMeasurement(const VpLinearDimensionEntity& dimension) noexcept;
// 返回标注类型的显示名称。
QString dimensionTypeName(VpDimensionType dimension_type);
// 按样式精度生成标注的默认文字。
QString dimensionDefaultText(const VpLinearDimensionEntity& dimension, int linear_precision = 2,
                             int angular_precision = 2);
// 返回标注的参照点，用于绘制与捕捉。
std::vector<VpPoint2d> dimensionReferencePoints(const VpLinearDimensionEntity& dimension);
// 判断标注定义是否有效。
bool isDimensionValid(const VpLinearDimensionEntity& dimension) noexcept;

} // namespace Vp
