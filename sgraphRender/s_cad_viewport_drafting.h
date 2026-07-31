#pragma once

#include "s_geometry_types.h"

namespace vectorPath
{

struct SGridBasis
{
    SPoint2d first_axis;
    SPoint2d second_axis;
};

SGridBasis draftingGridBasis(double rotation_degrees);
SPoint2d snapPointToDraftingGrid(const SPoint2d& point, double spacing, const SGridBasis& basis);

} // namespace vectorPath
