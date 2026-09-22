#include "vp_toolpath_document.h"

#include "vp_cad_document.h"
#include "vp_document_transaction.h"

#include <algorithm>
#include <unordered_set>

namespace Vp
{

namespace
{

struct VpLayerToolpathCandidates
{
    QString layer_name;
    std::vector<VpEntityRecord> entities;
};

VpToolpathSortResult sortToolpathsByLayer(const std::vector<VpEntityRecord>& candidates,
                                          const VpToolpathSortOptions& options)
{
    std::vector<VpLayerToolpathCandidates> layer_groups;
    for (const VpEntityRecord& entity : candidates)
    {
        const auto group = std::find_if(layer_groups.begin(), layer_groups.end(),
                                        [&entity](const VpLayerToolpathCandidates& candidate_group)
                                        {
                                            return candidate_group.layer_name.compare(
                                                       entity.layer_name, Qt::CaseInsensitive) == 0;
                                        });
        if (group != layer_groups.end())
        {
            group->entities.push_back(entity);
        }
        else
        {
            layer_groups.push_back({entity.layer_name, {entity}});
        }
    }

    VpToolpathSortResult combined_result;
    VpToolpathSortOptions layer_options = options;
    combined_result.entities.reserve(candidates.size());
    for (const VpLayerToolpathCandidates& layer_group : layer_groups)
    {
        VpToolpathSortResult layer_result =
            sortToolpathEntities(layer_group.entities, layer_options);
        if (options.mode == VpToolpathSortMode::Shortest && !layer_result.entities.empty())
        {
            layer_options.shortest_start = toolpathEndPoint(layer_result.entities.back());
        }
        combined_result.entities.insert(combined_result.entities.end(),
                                        layer_result.entities.begin(), layer_result.entities.end());
        combined_result.reversed_entity_ids.insert(combined_result.reversed_entity_ids.end(),
                                                   layer_result.reversed_entity_ids.begin(),
                                                   layer_result.reversed_entity_ids.end());
    }
    return combined_result;
}

} // namespace

VpToolpathDocumentSortResult
sortDocumentToolpaths(VpCadDocument& document, const std::vector<VpEntityId>& selected_entity_ids,
                      const VpToolpathSortOptions& options)
{
    const std::unordered_set<VpEntityId> selection(selected_entity_ids.begin(),
                                                   selected_entity_ids.end());
    std::vector<VpEntityRecord> candidates;
    std::vector<std::size_t> occupied_positions;
    const bool use_selection = !selection.empty();
    const auto& entities = document.entities();
    for (std::size_t index = 0; index < entities.size(); ++index)
    {
        const VpEntityRecord& entity = entities[index];
        if (isMachinableEntity(entity) &&
            (!use_selection || selection.find(entity.id) != selection.end()))
        {
            candidates.push_back(entity);
            occupied_positions.push_back(index);
        }
    }
    if (candidates.size() < 2)
    {
        return {};
    }

    const VpToolpathSortResult sorted = sortToolpathsByLayer(candidates, options);
    std::vector<VpEntityId> final_order;
    final_order.reserve(entities.size());
    for (const VpEntityRecord& entity : entities)
    {
        final_order.push_back(entity.id);
    }
    for (std::size_t index = 0; index < occupied_positions.size(); ++index)
    {
        final_order[occupied_positions[index]] = sorted.entities[index].id;
    }

    std::unique_ptr<VpDocumentTransaction> transaction =
        document.beginTransaction(QObject::tr("刀路排序"));
    const std::unordered_set<VpEntityId> reversed_ids(sorted.reversed_entity_ids.begin(),
                                                      sorted.reversed_entity_ids.end());
    for (const VpEntityRecord& entity : sorted.entities)
    {
        if (reversed_ids.find(entity.id) != reversed_ids.end())
        {
            transaction->replaceEntity(entity.id, entity);
        }
    }
    transaction->setEntityOrder(std::move(final_order));
    transaction->commit();
    return {static_cast<int>(sorted.entities.size()),
            static_cast<int>(sorted.reversed_entity_ids.size())};
}

} // namespace Vp
