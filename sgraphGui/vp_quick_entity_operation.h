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

struct VpQuickEntityOperationResult
{
    int target_count = 0;
    int changed_count = 0;
    std::vector<VpEntityId> created_entity_ids;
};

std::vector<VpEntityId> quickOperationTargetIds(const VpCadDocument& document,
                                                const std::vector<VpEntityId>& selected_entity_ids);
VpQuickEntityOperationResult
applyQuickEntityOperation(VpCadDocument& document,
                          const std::vector<VpEntityId>& selected_entity_ids,
                          VpQuickEntityOperation operation, const VpPoint2d& operation_center);

} // namespace Vp
