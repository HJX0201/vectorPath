#pragma once

#include "vp_entity.h"

#include <vector>

namespace Vp
{

enum class VpToolpathHorizontalDirection
{
    LeftToRight,
    RightToLeft
};

enum class VpToolpathVerticalDirection
{
    TopToBottom,
    BottomToTop
};

enum class VpToolpathSortMode
{
    RowScan,
    Shortest
};

struct VpToolpathSortOptions
{
    VpToolpathSortMode mode = VpToolpathSortMode::RowScan;
    VpToolpathHorizontalDirection horizontal = VpToolpathHorizontalDirection::LeftToRight;
    VpToolpathVerticalDirection vertical = VpToolpathVerticalDirection::TopToBottom;
    bool allow_reverse = false;
    VpPoint2d shortest_start;
};

struct VpToolpathSortResult
{
    std::vector<VpEntityRecord> entities;
    std::vector<VpEntityId> reversed_entity_ids;
};

enum class VpToolpathMotionType
{
    Rapid,
    Cutting
};

struct VpToolpathMotion
{
    VpToolpathMotionType type = VpToolpathMotionType::Rapid;
    VpPoint2d start_point;
    VpPoint2d end_point;
    VpEntityId entity_id = 0;
};

bool isMachinableEntity(const VpEntityRecord& entity) noexcept;
VpPoint2d toolpathStartPoint(const VpEntityRecord& entity) noexcept;
VpPoint2d toolpathEndPoint(const VpEntityRecord& entity) noexcept;
bool canReverseToolpath(const VpEntityRecord& entity) noexcept;
VpEntityRecord reversedToolpathEntity(const VpEntityRecord& entity);
VpToolpathSortResult sortToolpathEntities(const std::vector<VpEntityRecord>& entities,
                                          const VpToolpathSortOptions& options);
std::vector<VpToolpathMotion> generateToolpathMotions(const std::vector<VpEntityRecord>& entities,
                                                      const VpPoint2d& initial_position);

} // namespace Vp
