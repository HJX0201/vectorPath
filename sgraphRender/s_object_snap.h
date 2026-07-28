#pragma once

#include "s_entity.h"

#include <optional>
#include <cstdint>
#include <vector>

namespace smartGraphics
{

enum class SObjectSnapType
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

enum class SObjectSnapMode : std::uint8_t
{
    Endpoint = 0x01,
    Center = 0x02
};

using SObjectSnapModes = std::uint8_t;

constexpr SObjectSnapModes objectSnapModeValue(SObjectSnapMode mode) noexcept
{
    return static_cast<SObjectSnapModes>(mode);
}

constexpr bool objectSnapModeEnabled(SObjectSnapModes modes, SObjectSnapMode mode) noexcept
{
    return (modes & objectSnapModeValue(mode)) != 0;
}

struct SObjectSnapResult
{
    SPoint2d point;
    SObjectSnapType type = SObjectSnapType::Nearest;
};

std::optional<SObjectSnapResult> findObjectSnap(const std::vector<const SEntityRecord*>& entities,
                                                const SPoint2d& cursor,
                                                const std::optional<SPoint2d>& reference_point,
                                                double tolerance,
                                                SObjectSnapModes modes =
                                                    objectSnapModeValue(SObjectSnapMode::Endpoint) |
                                                    objectSnapModeValue(SObjectSnapMode::Center));

} // namespace smartGraphics
