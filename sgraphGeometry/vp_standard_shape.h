#pragma once

#include "vp_geometry_types.h"

#include <vector>

namespace Vp
{

enum class VpStandardShapeType
{
    FivePointStar,
    Triangle,
    Pentagon,
    Hexagon,
    Octagon,
    Diamond
};

std::vector<VpPoint2d> standardShapeVertices(VpStandardShapeType shape_type,
                                             const VpPoint2d& center,
                                             const VpPoint2d& radius_point);

} // namespace Vp
