#pragma once

#include "s_geometry_types.h"

#include <vector>

namespace smartCam
{

enum class SStandardShapeType
{
    FivePointStar,
    Triangle,
    Pentagon,
    Hexagon,
    Octagon,
    Diamond
};

std::vector<SPoint2d> standardShapeVertices(SStandardShapeType shape_type,
                                            const SPoint2d& center,
                                            const SPoint2d& radius_point);

} // namespace smartCam
