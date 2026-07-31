#pragma once

#include "s_geometry_types.h"
#include "s_result.h"

#include <cstdint>
#include <vector>

namespace vectorPath
{

enum class SPolygonBooleanOperation : std::uint8_t
{
    Union,
    Intersection,
    Difference,
    Xor,
    Complement
};

using SPolygonPath = std::vector<SPoint2d>;
using SPolygonPaths = std::vector<SPolygonPath>;

SResult<SPolygonPaths> polygonBoolean(const SPolygonPaths& subject_paths,
                                      const SPolygonPaths& clip_paths,
                                      SPolygonBooleanOperation operation);

} // namespace vectorPath
