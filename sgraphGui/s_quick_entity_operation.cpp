#include "s_quick_entity_operation.h"

#include "s_cad_document.h"
#include "s_cad_viewport_geometry.h"
#include "s_document_transaction.h"
#include "s_toolpath.h"

#include <algorithm>
#include <unordered_set>

namespace vectorPath
{
namespace
{

double rotationAngle(SQuickEntityOperation operation)
{
    switch (operation)
    {
    case SQuickEntityOperation::RotateClockwise90:
        return -90.0;
    case SQuickEntityOperation::RotateClockwise180:
        return -180.0;
    case SQuickEntityOperation::RotateCounterclockwise90:
        return 90.0;
    case SQuickEntityOperation::RotateCounterclockwise180:
        return 180.0;
    default:
        return 0.0;
    }
}

const SEntityRecord* entityById(const SCadDocument& document, SEntityId entity_id)
{
    const auto iterator =
        std::find_if(document.entities().begin(), document.entities().end(),
                     [entity_id](const SEntityRecord& entity)
                     {
                         return entity.id == entity_id;
                     });
    return iterator == document.entities().end() ? nullptr : &*iterator;
}

std::vector<SEntityId> reversedEntityOrder(
    const SCadDocument& document, const std::vector<SEntityId>& target_ids, bool use_selection)
{
    std::vector<SEntityId> reversed_targets(target_ids.rbegin(), target_ids.rend());
    if (!use_selection)
    {
        return reversed_targets;
    }
    const std::unordered_set<SEntityId> targets(target_ids.begin(), target_ids.end());
    std::vector<SEntityId> result;
    result.reserve(document.entities().size());
    std::size_t insertion_index = 0;
    bool found_first_target = false;
    for (const SEntityRecord& entity : document.entities())
    {
        if (targets.find(entity.id) != targets.end())
        {
            found_first_target = true;
            continue;
        }
        if (!found_first_target)
        {
            ++insertion_index;
        }
        result.push_back(entity.id);
    }
    result.insert(result.begin() + static_cast<std::ptrdiff_t>(insertion_index),
                  reversed_targets.begin(), reversed_targets.end());
    return result;
}

SQuickEntityOperationResult reverseEntities(
    SCadDocument& document, const std::vector<SEntityId>& target_ids, bool use_selection)
{
    SQuickEntityOperationResult result;
    result.target_count = static_cast<int>(target_ids.size());
    std::unique_ptr<SDocumentTransaction> transaction =
        document.beginTransaction(QObject::tr("反向实体与顺序"));
    for (SEntityId entity_id : target_ids)
    {
        const SEntityRecord* source = entityById(document, entity_id);
        if (source && canReverseToolpath(*source))
        {
            transaction->replaceEntity(entity_id, reversedToolpathEntity(*source));
            ++result.changed_count;
        }
    }
    const std::vector<SEntityId> order = reversedEntityOrder(document, target_ids, use_selection);
    std::vector<SEntityId> original_order;
    original_order.reserve(document.entities().size());
    for (const SEntityRecord& entity : document.entities())
    {
        original_order.push_back(entity.id);
    }
    if (order != original_order)
    {
        transaction->setEntityOrder(order);
        result.changed_count = std::max(result.changed_count, 1);
    }
    if (result.changed_count > 0)
    {
        transaction->commit();
    }
    return result;
}

} // namespace

std::vector<SEntityId> quickOperationTargetIds(
    const SCadDocument& document, const std::vector<SEntityId>& selected_entity_ids)
{
    const std::unordered_set<SEntityId> selected(selected_entity_ids.begin(),
                                                  selected_entity_ids.end());
    std::vector<SEntityId> result;
    for (const SEntityRecord& entity : document.entities())
    {
        if (selected.empty() || selected.find(entity.id) != selected.end())
        {
            result.push_back(entity.id);
        }
    }
    return result;
}

SQuickEntityOperationResult applyQuickEntityOperation(
    SCadDocument& document, const std::vector<SEntityId>& selected_entity_ids,
    SQuickEntityOperation operation, const SPoint2d& operation_center)
{
    const std::vector<SEntityId> target_ids =
        quickOperationTargetIds(document, selected_entity_ids);
    if (target_ids.empty())
    {
        return {};
    }
    if (operation == SQuickEntityOperation::Reverse)
    {
        return reverseEntities(document, target_ids, !selected_entity_ids.empty());
    }

    SQuickEntityOperationResult result;
    result.target_count = static_cast<int>(target_ids.size());
    const bool is_mirror = operation == SQuickEntityOperation::MirrorVertical ||
                           operation == SQuickEntityOperation::MirrorHorizontal;
    std::unique_ptr<SDocumentTransaction> transaction = document.beginTransaction(
        is_mirror ? QObject::tr("快速镜像实体") : QObject::tr("快速旋转实体"));
    const SPoint2d axis_start =
        operation == SQuickEntityOperation::MirrorVertical
            ? SPoint2d{operation_center.x, operation_center.y - 1.0}
            : SPoint2d{operation_center.x - 1.0, operation_center.y};
    const SPoint2d axis_end =
        operation == SQuickEntityOperation::MirrorVertical
            ? SPoint2d{operation_center.x, operation_center.y + 1.0}
            : SPoint2d{operation_center.x + 1.0, operation_center.y};
    for (SEntityId entity_id : target_ids)
    {
        const SEntityRecord* source = entityById(document, entity_id);
        if (!source)
        {
            continue;
        }
        if (is_mirror)
        {
            result.created_entity_ids.push_back(
                transaction->addEntityCopy(mirroredEntity(*source, axis_start, axis_end)));
        }
        else
        {
            SEntityRecord replacement =
                rotatedEntity(*source, operation_center, rotationAngle(operation));
            replacement.associative_array.reset();
            transaction->replaceEntity(entity_id, std::move(replacement));
        }
        ++result.changed_count;
    }
    if (result.changed_count > 0)
    {
        transaction->commit();
    }
    return result;
}

} // namespace vectorPath
