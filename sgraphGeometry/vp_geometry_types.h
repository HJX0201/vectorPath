#pragma once

#include <cmath>

namespace Vp
{

struct VpPoint2d
{
    double x = 0.0;
    double y = 0.0;
};

double distance(const VpPoint2d& first_point, const VpPoint2d& second_point) noexcept;

} // namespace Vp
