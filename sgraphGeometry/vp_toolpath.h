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

// 刀具路径排序选项：扫描方向、起点与是否允许反向。
struct VpToolpathSortOptions
{
    VpToolpathSortMode mode = VpToolpathSortMode::RowScan;
    VpToolpathHorizontalDirection horizontal = VpToolpathHorizontalDirection::LeftToRight;
    VpToolpathVerticalDirection vertical = VpToolpathVerticalDirection::TopToBottom;
    bool allow_reverse = false;
    VpPoint2d shortest_start;
};

// 排序后的图元序列与被反向的图元编号。
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

// 刀具路径中的一段运动：快速定位或切削。
struct VpToolpathMotion
{
    VpToolpathMotionType type = VpToolpathMotionType::Rapid;
    VpPoint2d start_point;
    VpPoint2d end_point;
    VpEntityId entity_id = 0;
};

// 判断图元是否可加工。
bool isMachinableEntity(const VpEntityRecord& entity) noexcept;
// 返回走刀起点。
VpPoint2d toolpathStartPoint(const VpEntityRecord& entity) noexcept;
// 返回走刀终点。
VpPoint2d toolpathEndPoint(const VpEntityRecord& entity) noexcept;
// 判断图元能否反向加工。
bool canReverseToolpath(const VpEntityRecord& entity) noexcept;
// 返回反向后的图元副本。
VpEntityRecord reversedToolpathEntity(const VpEntityRecord& entity);
// 按选项对图元排序。
VpToolpathSortResult sortToolpathEntities(const std::vector<VpEntityRecord>& entities,
                                          const VpToolpathSortOptions& options);
// 把排序后的图元展开为运动段。
std::vector<VpToolpathMotion> generateToolpathMotions(const std::vector<VpEntityRecord>& entities,
                                                      const VpPoint2d& initial_position);

} // namespace Vp
