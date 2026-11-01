#pragma once

#include "vp_geometry_types.h"
#include "vp_result.h"

#include <cstdint>
#include <vector>

namespace Vp
{

enum class VpPolygonBooleanOperation : std::uint8_t
{
    Union,
    Intersection,
    Difference,
    Xor,
    Complement
};

using VpPolygonPath = std::vector<VpPoint2d>;
using VpPolygonPaths = std::vector<VpPolygonPath>;

// 对多边形做并、交、差布尔运算。
VpResult<VpPolygonPaths> polygonBoolean(const VpPolygonPaths& subject_paths,
                                        const VpPolygonPaths& clip_paths,
                                        VpPolygonBooleanOperation operation);

} // namespace Vp
