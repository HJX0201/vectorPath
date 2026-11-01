#pragma once

#include "vp_entity.h"

#include <vector>

namespace Vp
{

class VpCadDocument;

enum class VpQuickEntityOperation
{
    Reverse,
    RotateClockwise90,
    RotateClockwise180,
    RotateCounterclockwise90,
    RotateCounterclockwise180,
    MirrorVertical,
    MirrorHorizontal
};

// 批量图元操作的执行统计。
struct VpQuickEntityOperationResult
{
    int target_count = 0;
    int changed_count = 0;
    std::vector<VpEntityId> created_entity_ids;
};

// 返回快捷操作要作用的对象集合。
std::vector<VpEntityId> quickOperationTargetIds(const VpCadDocument& document,
                                                const std::vector<VpEntityId>& selected_entity_ids);
// 对选中对象执行快捷操作并返回统计。
VpQuickEntityOperationResult
applyQuickEntityOperation(VpCadDocument& document,
                          const std::vector<VpEntityId>& selected_entity_ids,
                          VpQuickEntityOperation operation, const VpPoint2d& operation_center);

} // namespace Vp
