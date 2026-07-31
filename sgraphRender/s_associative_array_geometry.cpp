#include "s_associative_array_geometry.h"

#include "s_cad_viewport_geometry.h"

#include <algorithm>
#include <iterator>
#include <map>

namespace vectorPath
{
namespace
{

void applyArrayData(std::vector<SEntityRecord>& results, const std::vector<SEntityRecord>& sources,
                    const SAssociativeArrayData& parameters)
{
    if (sources.empty())
    {
        return;
    }
    for (std::size_t index = 0; index < results.size(); ++index)
    {
        const std::size_t source_index = index % sources.size();
        SAssociativeArrayData member_data = parameters;
        member_data.source_index = static_cast<std::uint32_t>(source_index);
        member_data.item_index = static_cast<std::uint32_t>(index / sources.size());
        member_data.source_type = sources[source_index].type;
        member_data.source_geometry = sources[source_index].geometry;
        results[index].associative_array = std::move(member_data);
    }
}

std::vector<SEntityRecord> sourceEntities(const std::vector<SEntityRecord>& document_entities,
                                          SArrayId array_id)
{
    std::map<std::uint32_t, SEntityRecord> indexed_sources;
    for (const SEntityRecord& member : document_entities)
    {
        if (!member.associative_array || member.associative_array->array_id != array_id)
        {
            continue;
        }
        const SAssociativeArrayData& data = *member.associative_array;
        SEntityRecord source = member;
        source.type = data.source_type;
        source.geometry = data.source_geometry;
        source.associative_array.reset();
        indexed_sources.emplace(data.source_index, std::move(source));
    }
    std::vector<SEntityRecord> sources;
    sources.reserve(indexed_sources.size());
    for (auto& item : indexed_sources)
    {
        sources.push_back(std::move(item.second));
    }
    return sources;
}

std::vector<SEntityId> sortedMemberIds(const std::vector<SEntityRecord>& document_entities,
                                       SArrayId array_id)
{
    std::vector<const SEntityRecord*> members;
    for (const SEntityRecord& entity : document_entities)
    {
        if (entity.associative_array && entity.associative_array->array_id == array_id)
        {
            members.push_back(&entity);
        }
    }
    std::sort(
        members.begin(), members.end(),
        [](const auto* first, const auto* second)
        {
            if (first->associative_array->item_index != second->associative_array->item_index)
            {
                return first->associative_array->item_index < second->associative_array->item_index;
            }
            return first->associative_array->source_index < second->associative_array->source_index;
        });
    std::vector<SEntityId> ids;
    ids.reserve(members.size());
    for (const SEntityRecord* member : members)
    {
        ids.push_back(member->id);
    }
    return ids;
}

const SEntityRecord* pathEntity(const std::vector<SEntityRecord>& entities, SEntityId path_id)
{
    const auto iterator = std::find_if(entities.begin(), entities.end(),
                                       [path_id](const SEntityRecord& entity)
                                       {
                                           return entity.id == path_id;
                                       });
    return iterator == entities.end() ? nullptr : &*iterator;
}

} // namespace

std::vector<SEntityRecord>
associativeRectangularArrayEntities(const std::vector<SEntityRecord>& sources, SArrayId array_id,
                                    double column_spacing, double row_spacing, int column_count,
                                    int row_count)
{
    std::vector<SEntityRecord> results = sources;
    std::vector<SEntityRecord> copies =
        rectangularArrayEntities(sources, column_spacing, row_spacing, column_count, row_count);
    results.insert(results.end(), std::make_move_iterator(copies.begin()),
                   std::make_move_iterator(copies.end()));
    SAssociativeArrayData parameters;
    parameters.array_id = array_id;
    parameters.array_type = SArrayType::Rectangular;
    parameters.column_count = column_count;
    parameters.row_count = row_count;
    parameters.item_count = column_count * row_count;
    parameters.column_spacing = column_spacing;
    parameters.row_spacing = row_spacing;
    applyArrayData(results, sources, parameters);
    return results;
}

std::vector<SEntityRecord> associativePolarArrayEntities(const std::vector<SEntityRecord>& sources,
                                                         SArrayId array_id, const SPoint2d& center,
                                                         int item_count, double fill_angle)
{
    std::vector<SEntityRecord> results = sources;
    std::vector<SEntityRecord> copies = polarArrayEntities(sources, center, item_count, fill_angle);
    results.insert(results.end(), std::make_move_iterator(copies.begin()),
                   std::make_move_iterator(copies.end()));
    SAssociativeArrayData parameters;
    parameters.array_id = array_id;
    parameters.array_type = SArrayType::Polar;
    parameters.item_count = item_count;
    parameters.fill_angle = fill_angle;
    parameters.center = center;
    applyArrayData(results, sources, parameters);
    return results;
}

std::vector<SEntityRecord> associativePathArrayEntities(const std::vector<SEntityRecord>& sources,
                                                        SArrayId array_id,
                                                        const SEntityRecord& path,
                                                        const SPoint2d& source_base_point,
                                                        int item_count, bool align_to_path)
{
    std::vector<SEntityRecord> results =
        pathArrayEntities(sources, path, source_base_point, item_count, align_to_path);
    SAssociativeArrayData parameters;
    parameters.array_id = array_id;
    parameters.array_type = SArrayType::Path;
    parameters.item_count = item_count;
    parameters.source_base_point = source_base_point;
    parameters.path_entity_id = path.id;
    parameters.align_to_path = align_to_path;
    applyArrayData(results, sources, parameters);
    return results;
}

bool regenerateAssociativeArray(const std::vector<SEntityRecord>& document_entities,
                                const SAssociativeArrayData& parameters,
                                std::vector<SEntityRecord>& results,
                                std::vector<SEntityId>& existing_ids)
{
    const std::vector<SEntityRecord> sources =
        sourceEntities(document_entities, parameters.array_id);
    if (sources.empty())
    {
        return false;
    }
    if (parameters.array_type == SArrayType::Rectangular)
    {
        results = associativeRectangularArrayEntities(
            sources, parameters.array_id, parameters.column_spacing, parameters.row_spacing,
            parameters.column_count, parameters.row_count);
    }
    else if (parameters.array_type == SArrayType::Polar)
    {
        results = associativePolarArrayEntities(sources, parameters.array_id, parameters.center,
                                                parameters.item_count, parameters.fill_angle);
    }
    else
    {
        const SEntityRecord* path = pathEntity(document_entities, parameters.path_entity_id);
        if (!path)
        {
            return false;
        }
        results = associativePathArrayEntities(sources, parameters.array_id, *path,
                                               parameters.source_base_point, parameters.item_count,
                                               parameters.align_to_path);
    }
    existing_ids = sortedMemberIds(document_entities, parameters.array_id);
    return !results.empty() && !existing_ids.empty();
}

} // namespace vectorPath
