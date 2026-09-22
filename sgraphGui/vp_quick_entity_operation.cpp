#include "vp_quick_entity_operation.h"

#include "vp_cad_document.h"
#include "vp_cad_viewport_geometry.h"
#include "vp_document_transaction.h"
#include "vp_toolpath.h"

#include <algorithm>
#include <unordered_set>

namespace Vp
{
namespace
{

double rotationAngle(VpQuickEntityOperation operation)
{
    switch (operation)
    {
    case VpQuickEntityOperation::RotateClockwise90:
        return -90.0;
    case VpQuickEntityOperation::RotateClockwise180:
        return -180.0;
    case VpQuickEntityOperation::RotateCounterclockwise90:
        return 90.0;
    case VpQuickEntityOperation::RotateCounterclockwise180:
        return 180.0;
    default:
        return 0.0;
    }
}

const VpEntityRecord* entityById(const VpCadDocument& document, VpEntityId entity_id)
{
    const auto iterator = std::find_if(document.entities().begin(), document.entities().end(),
                                       [entity_id](const VpEntityRecord& entity)
                                       {
                                           return entity.id == entity_id;
                                       });
    return iterator == document.entities().end() ? nullptr : &*iterator;
}

std::vector<VpEntityId> reversedEntityOrder(const VpCadDocument& document,
                                            const std::vector<VpEntityId>& target_ids,
                                            bool use_selection)
{
    std::vector<VpEntityId> reversed_targets(target_ids.rbegin(), target_ids.rend());
    if (!use_selection)
    {
        return reversed_targets;
    }
    const std::unordered_set<VpEntityId> targets(target_ids.begin(), target_ids.end());
    std::vector<VpEntityId> result;
    result.reserve(document.entities().size());
    std::size_t insertion_index = 0;
    bool found_first_target = false;
    for (const VpEntityRecord& entity : document.entities())
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

VpQuickEntityOperationResult reverseEntities(VpCadDocument& document,
                                             const std::vector<VpEntityId>& target_ids,
                                             bool use_selection)
{
    VpQuickEntityOperationResult result;
    result.target_count = static_cast<int>(target_ids.size());
    std::unique_ptr<VpDocumentTransaction> transaction =
        document.beginTransaction(QObject::tr("反向实体与顺序"));
    for (VpEntityId entity_id : target_ids)
    {
        const VpEntityRecord* source = entityById(document, entity_id);
        if (source && canReverseToolpath(*source))
        {
            transaction->replaceEntity(entity_id, reversedToolpathEntity(*source));
            ++result.changed_count;
        }
    }
    const std::vector<VpEntityId> order = reversedEntityOrder(document, target_ids, use_selection);
    std::vector<VpEntityId> original_order;
    original_order.reserve(document.entities().size());
    for (const VpEntityRecord& entity : document.entities())
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

std::vector<VpEntityId> quickOperationTargetIds(const VpCadDocument& document,
                                                const std::vector<VpEntityId>& selected_entity_ids)
{
    const std::unordered_set<VpEntityId> selected(selected_entity_ids.begin(),
                                                  selected_entity_ids.end());
    std::vector<VpEntityId> result;
    for (const VpEntityRecord& entity : document.entities())
    {
        if (selected.empty() || selected.find(entity.id) != selected.end())
        {
            result.push_back(entity.id);
        }
    }
    return result;
}

VpQuickEntityOperationResult
applyQuickEntityOperation(VpCadDocument& document,
                          const std::vector<VpEntityId>& selected_entity_ids,
                          VpQuickEntityOperation operation, const VpPoint2d& operation_center)
{
    const std::vector<VpEntityId> target_ids =
        quickOperationTargetIds(document, selected_entity_ids);
    if (target_ids.empty())
    {
        return {};
    }
    if (operation == VpQuickEntityOperation::Reverse)
    {
        return reverseEntities(document, target_ids, !selected_entity_ids.empty());
    }

    VpQuickEntityOperationResult result;
    result.target_count = static_cast<int>(target_ids.size());
    const bool is_mirror = operation == VpQuickEntityOperation::MirrorVertical ||
                           operation == VpQuickEntityOperation::MirrorHorizontal;
    std::unique_ptr<VpDocumentTransaction> transaction = document.beginTransaction(
        is_mirror ? QObject::tr("快速镜像实体") : QObject::tr("快速旋转实体"));
    const VpPoint2d axis_start = operation == VpQuickEntityOperation::MirrorVertical
                                     ? VpPoint2d{operation_center.x, operation_center.y - 1.0}
                                     : VpPoint2d{operation_center.x - 1.0, operation_center.y};
    const VpPoint2d axis_end = operation == VpQuickEntityOperation::MirrorVertical
                                   ? VpPoint2d{operation_center.x, operation_center.y + 1.0}
                                   : VpPoint2d{operation_center.x + 1.0, operation_center.y};
    for (VpEntityId entity_id : target_ids)
    {
        const VpEntityRecord* source = entityById(document, entity_id);
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
            VpEntityRecord replacement =
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

} // namespace Vp
