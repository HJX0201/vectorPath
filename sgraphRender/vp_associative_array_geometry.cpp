#include "vp_associative_array_geometry.h"

#include "vp_cad_viewport_geometry.h"

#include <algorithm>
#include <iterator>
#include <map>

namespace Vp
{
namespace
{

void applyArrayData(std::vector<VpEntityRecord>& results,
                    const std::vector<VpEntityRecord>& sources,
                    const VpAssociativeArrayData& parameters)
{
    if (sources.empty())
    {
        return;
    }
    for (std::size_t index = 0; index < results.size(); ++index)
    {
        const std::size_t source_index = index % sources.size();
        VpAssociativeArrayData member_data = parameters;
        member_data.source_index = static_cast<std::uint32_t>(source_index);
        member_data.item_index = static_cast<std::uint32_t>(index / sources.size());
        member_data.source_type = sources[source_index].type;
        member_data.source_geometry = sources[source_index].geometry;
        results[index].associative_array = std::move(member_data);
    }
}

std::vector<VpEntityRecord> sourceEntities(const std::vector<VpEntityRecord>& document_entities,
                                           VpArrayId array_id)
{
    std::map<std::uint32_t, VpEntityRecord> indexed_sources;
    for (const VpEntityRecord& member : document_entities)
    {
        if (!member.associative_array || member.associative_array->array_id != array_id)
        {
            continue;
        }
        const VpAssociativeArrayData& data = *member.associative_array;
        VpEntityRecord source = member;
        source.type = data.source_type;
        source.geometry = data.source_geometry;
        source.associative_array.reset();
        indexed_sources.emplace(data.source_index, std::move(source));
    }
    std::vector<VpEntityRecord> sources;
    sources.reserve(indexed_sources.size());
    for (auto& item : indexed_sources)
    {
        sources.push_back(std::move(item.second));
    }
    return sources;
}

std::vector<VpEntityId> sortedMemberIds(const std::vector<VpEntityRecord>& document_entities,
                                        VpArrayId array_id)
{
    std::vector<const VpEntityRecord*> members;
    for (const VpEntityRecord& entity : document_entities)
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
    std::vector<VpEntityId> ids;
    ids.reserve(members.size());
    for (const VpEntityRecord* member : members)
    {
        ids.push_back(member->id);
    }
    return ids;
}

const VpEntityRecord* pathEntity(const std::vector<VpEntityRecord>& entities, VpEntityId path_id)
{
    const auto iterator = std::find_if(entities.begin(), entities.end(),
                                       [path_id](const VpEntityRecord& entity)
                                       {
                                           return entity.id == path_id;
                                       });
    return iterator == entities.end() ? nullptr : &*iterator;
}

} // namespace

std::vector<VpEntityRecord>
associativeRectangularArrayEntities(const std::vector<VpEntityRecord>& sources, VpArrayId array_id,
                                    double column_spacing, double row_spacing, int column_count,
                                    int row_count)
{
    std::vector<VpEntityRecord> results = sources;
    std::vector<VpEntityRecord> copies =
        rectangularArrayEntities(sources, column_spacing, row_spacing, column_count, row_count);
    results.insert(results.end(), std::make_move_iterator(copies.begin()),
                   std::make_move_iterator(copies.end()));
    VpAssociativeArrayData parameters;
    parameters.array_id = array_id;
    parameters.array_type = VpArrayType::Rectangular;
    parameters.column_count = column_count;
    parameters.row_count = row_count;
    parameters.item_count = column_count * row_count;
    parameters.column_spacing = column_spacing;
    parameters.row_spacing = row_spacing;
    applyArrayData(results, sources, parameters);
    return results;
}

std::vector<VpEntityRecord>
associativePolarArrayEntities(const std::vector<VpEntityRecord>& sources, VpArrayId array_id,
                              const VpPoint2d& center, int item_count, double fill_angle)
{
    std::vector<VpEntityRecord> results = sources;
    std::vector<VpEntityRecord> copies =
        polarArrayEntities(sources, center, item_count, fill_angle);
    results.insert(results.end(), std::make_move_iterator(copies.begin()),
                   std::make_move_iterator(copies.end()));
    VpAssociativeArrayData parameters;
    parameters.array_id = array_id;
    parameters.array_type = VpArrayType::Polar;
    parameters.item_count = item_count;
    parameters.fill_angle = fill_angle;
    parameters.center = center;
    applyArrayData(results, sources, parameters);
    return results;
}

std::vector<VpEntityRecord> associativePathArrayEntities(const std::vector<VpEntityRecord>& sources,
                                                         VpArrayId array_id,
                                                         const VpEntityRecord& path,
                                                         const VpPoint2d& source_base_point,
                                                         int item_count, bool align_to_path)
{
    std::vector<VpEntityRecord> results =
        pathArrayEntities(sources, path, source_base_point, item_count, align_to_path);
    VpAssociativeArrayData parameters;
    parameters.array_id = array_id;
    parameters.array_type = VpArrayType::Path;
    parameters.item_count = item_count;
    parameters.source_base_point = source_base_point;
    parameters.path_entity_id = path.id;
    parameters.align_to_path = align_to_path;
    applyArrayData(results, sources, parameters);
    return results;
}

bool regenerateAssociativeArray(const std::vector<VpEntityRecord>& document_entities,
                                const VpAssociativeArrayData& parameters,
                                std::vector<VpEntityRecord>& results,
                                std::vector<VpEntityId>& existing_ids)
{
    const std::vector<VpEntityRecord> sources =
        sourceEntities(document_entities, parameters.array_id);
    if (sources.empty())
    {
        return false;
    }
    if (parameters.array_type == VpArrayType::Rectangular)
    {
        results = associativeRectangularArrayEntities(
            sources, parameters.array_id, parameters.column_spacing, parameters.row_spacing,
            parameters.column_count, parameters.row_count);
    }
    else if (parameters.array_type == VpArrayType::Polar)
    {
        results = associativePolarArrayEntities(sources, parameters.array_id, parameters.center,
                                                parameters.item_count, parameters.fill_angle);
    }
    else
    {
        const VpEntityRecord* path = pathEntity(document_entities, parameters.path_entity_id);
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

} // namespace Vp
