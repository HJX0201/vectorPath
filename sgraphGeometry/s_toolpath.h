#pragma once

#include "s_entity.h"

#include <vector>

namespace smartCam
{

enum class SToolpathHorizontalDirection
{
    LeftToRight,
    RightToLeft
};

enum class SToolpathVerticalDirection
{
    TopToBottom,
    BottomToTop
};

enum class SToolpathSortMode
{
    RowScan,
    Shortest
};

struct SToolpathSortOptions
{
    SToolpathSortMode mode = SToolpathSortMode::RowScan;
    SToolpathHorizontalDirection horizontal = SToolpathHorizontalDirection::LeftToRight;
    SToolpathVerticalDirection vertical = SToolpathVerticalDirection::TopToBottom;
    bool allow_reverse = false;
    SPoint2d shortest_start;
};

struct SToolpathSortResult
{
    std::vector<SEntityRecord> entities;
    std::vector<SEntityId> reversed_entity_ids;
};

enum class SToolpathMotionType
{
    Rapid,
    Cutting
};

struct SToolpathMotion
{
    SToolpathMotionType type = SToolpathMotionType::Rapid;
    SPoint2d start_point;
    SPoint2d end_point;
    SEntityId entity_id = 0;
};

bool isMachinableEntity(const SEntityRecord& entity) noexcept;
SPoint2d toolpathStartPoint(const SEntityRecord& entity) noexcept;
SPoint2d toolpathEndPoint(const SEntityRecord& entity) noexcept;
bool canReverseToolpath(const SEntityRecord& entity) noexcept;
SEntityRecord reversedToolpathEntity(const SEntityRecord& entity);
SToolpathSortResult sortToolpathEntities(const std::vector<SEntityRecord>& entities,
                                         const SToolpathSortOptions& options);
std::vector<SToolpathMotion> generateToolpathMotions(
    const std::vector<SEntityRecord>& entities, const SPoint2d& initial_position);

} // namespace smartCam
