#pragma once

#include "vp_entity.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace Vp
{

enum class VpObjectSnapType
{
    Endpoint,
    Midpoint,
    Center,
    Quadrant,
    Intersection,
    ApparentIntersection,
    Extension,
    Parallel,
    Tracking,
    Perpendicular,
    Tangent,
    Nearest
};

enum class VpObjectSnapMode : std::uint8_t
{
    Endpoint = 0x01,
    Center = 0x02
};

using VpObjectSnapModes = std::uint8_t;

// 把单个捕捉模式转换为位标志。
constexpr VpObjectSnapModes objectSnapModeValue(VpObjectSnapMode mode) noexcept
{
    return static_cast<VpObjectSnapModes>(mode);
}

// 判断位标志中是否包含指定捕捉模式。
constexpr bool objectSnapModeEnabled(VpObjectSnapModes modes, VpObjectSnapMode mode) noexcept
{
    return (modes & objectSnapModeValue(mode)) != 0;
}

// 对象捕捉的命中点与捕捉类型。
struct VpObjectSnapResult
{
    VpPoint2d point;
    VpObjectSnapType type = VpObjectSnapType::Nearest;
};

// 在候选图元中查找最合适的捕捉点。
std::optional<VpObjectSnapResult>
findObjectSnap(const std::vector<const VpEntityRecord*>& entities, const VpPoint2d& cursor,
               const std::optional<VpPoint2d>& reference_point, double tolerance,
               VpObjectSnapModes modes = objectSnapModeValue(VpObjectSnapMode::Endpoint) |
                                         objectSnapModeValue(VpObjectSnapMode::Center));

} // namespace Vp
