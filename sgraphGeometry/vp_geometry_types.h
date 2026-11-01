#pragma once

#include <cmath>

namespace Vp
{

// 二维坐标点，几何与文档层共用的基础类型。
struct VpPoint2d
{
    double x = 0.0;
    double y = 0.0;
};

// 计算两点间距离。
double distance(const VpPoint2d& first_point, const VpPoint2d& second_point) noexcept;

} // namespace Vp
