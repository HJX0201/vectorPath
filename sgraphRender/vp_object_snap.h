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

constexpr VpObjectSnapModes objectSnapModeValue(VpObjectSnapMode mode) noexcept
{
    return static_cast<VpObjectSnapModes>(mode);
}

constexpr bool objectSnapModeEnabled(VpObjectSnapModes modes, VpObjectSnapMode mode) noexcept
{
    return (modes & objectSnapModeValue(mode)) != 0;
}

struct VpObjectSnapResult
{
    VpPoint2d point;
    VpObjectSnapType type = VpObjectSnapType::Nearest;
};

std::optional<VpObjectSnapResult>
findObjectSnap(const std::vector<const VpEntityRecord*>& entities, const VpPoint2d& cursor,
               const std::optional<VpPoint2d>& reference_point, double tolerance,
               VpObjectSnapModes modes = objectSnapModeValue(VpObjectSnapMode::Endpoint) |
                                         objectSnapModeValue(VpObjectSnapMode::Center));

} // namespace Vp
