#include "s_toolpath_document.h"

#include "s_cad_document.h"
#include "s_document_transaction.h"

#include <algorithm>
#include <unordered_set>

namespace smartGraphics
{

namespace
{

struct SLayerToolpathCandidates
{
    QString layer_name;
    std::vector<SEntityRecord> entities;
};

SToolpathSortResult sortToolpathsByLayer(
    const std::vector<SEntityRecord>& candidates, const SToolpathSortOptions& options)
{
    std::vector<SLayerToolpathCandidates> layer_groups;
    for (const SEntityRecord& entity : candidates)
    {
        const auto group = std::find_if(
            layer_groups.begin(), layer_groups.end(),
            [&entity](const SLayerToolpathCandidates& candidate_group)
            {
                return candidate_group.layer_name.compare(entity.layer_name, Qt::CaseInsensitive) ==
                       0;
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

    SToolpathSortResult combined_result;
    SToolpathSortOptions layer_options = options;
    combined_result.entities.reserve(candidates.size());
    for (const SLayerToolpathCandidates& layer_group : layer_groups)
    {
        SToolpathSortResult layer_result =
            sortToolpathEntities(layer_group.entities, layer_options);
        if (options.mode == SToolpathSortMode::Shortest && !layer_result.entities.empty())
        {
            layer_options.shortest_start = toolpathEndPoint(layer_result.entities.back());
        }
        combined_result.entities.insert(combined_result.entities.end(),
                                        layer_result.entities.begin(), layer_result.entities.end());
        combined_result.reversed_entity_ids.insert(
            combined_result.reversed_entity_ids.end(), layer_result.reversed_entity_ids.begin(),
            layer_result.reversed_entity_ids.end());
    }
    return combined_result;
}

} // namespace

SToolpathDocumentSortResult sortDocumentToolpaths(
    SCadDocument& document, const std::vector<SEntityId>& selected_entity_ids,
    const SToolpathSortOptions& options)
{
    const std::unordered_set<SEntityId> selection(selected_entity_ids.begin(),
                                                   selected_entity_ids.end());
    std::vector<SEntityRecord> candidates;
    std::vector<std::size_t> occupied_positions;
    const bool use_selection = !selection.empty();
    const auto& entities = document.entities();
    for (std::size_t index = 0; index < entities.size(); ++index)
    {
        const SEntityRecord& entity = entities[index];
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

    const SToolpathSortResult sorted = sortToolpathsByLayer(candidates, options);
    std::vector<SEntityId> final_order;
    final_order.reserve(entities.size());
    for (const SEntityRecord& entity : entities)
    {
        final_order.push_back(entity.id);
    }
    for (std::size_t index = 0; index < occupied_positions.size(); ++index)
    {
        final_order[occupied_positions[index]] = sorted.entities[index].id;
    }

    std::unique_ptr<SDocumentTransaction> transaction =
        document.beginTransaction(QObject::tr("刀路排序"));
    const std::unordered_set<SEntityId> reversed_ids(sorted.reversed_entity_ids.begin(),
                                                      sorted.reversed_entity_ids.end());
    for (const SEntityRecord& entity : sorted.entities)
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

} // namespace smartGraphics
