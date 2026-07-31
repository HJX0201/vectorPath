#pragma once

#include "s_entity.h"

#include <vector>

namespace smartCam
{

class SCadDocument;

enum class SQuickEntityOperation
{
    Reverse,
    RotateClockwise90,
    RotateClockwise180,
    RotateCounterclockwise90,
    RotateCounterclockwise180,
    MirrorVertical,
    MirrorHorizontal
};

struct SQuickEntityOperationResult
{
    int target_count = 0;
    int changed_count = 0;
    std::vector<SEntityId> created_entity_ids;
};

std::vector<SEntityId> quickOperationTargetIds(
    const SCadDocument& document, const std::vector<SEntityId>& selected_entity_ids);
SQuickEntityOperationResult applyQuickEntityOperation(
    SCadDocument& document, const std::vector<SEntityId>& selected_entity_ids,
    SQuickEntityOperation operation, const SPoint2d& operation_center);

} // namespace smartCam
