#include "vp_cad_document.h"
#include "vp_id_collection.h"

#include <algorithm>
#include <unordered_map>
#include <utility>

namespace Vp
{
namespace
{

std::vector<VpEntityId> entityOrder(const std::vector<VpEntityRecord>& entities)
{
    std::vector<VpEntityId> order;
    order.reserve(entities.size());
    for (const VpEntityRecord& entity : entities)
    {
        order.push_back(entity.id);
    }
    return order;
}

void applyEntityOrder(std::vector<VpEntityRecord>& entities, const std::vector<VpEntityId>& order)
{
    std::unordered_map<VpEntityId, std::size_t> ranks;
    ranks.reserve(order.size());
    for (std::size_t index = 0; index < order.size(); ++index)
    {
        ranks.emplace(order[index], index);
    }
    std::stable_sort(entities.begin(), entities.end(),
                     [&ranks](const VpEntityRecord& first, const VpEntityRecord& second)
                     {
                         return ranks.at(first.id) < ranks.at(second.id);
                     });
}

} // namespace

void VpCadDocument::undo()
{
    if (m_undo_stack.empty())
    {
        return;
    }
    VpHistoryEntry entry = std::move(m_undo_stack.back());
    m_undo_stack.pop_back();
    eraseRecordsById(m_entities, entry.added_entities);
    m_entities.insert(m_entities.end(), entry.removed_entities.begin(),
                      entry.removed_entities.end());
    if (entry.has_entity_order_change)
    {
        applyEntityOrder(m_entities, entry.entity_order_before);
    }
    if (entry.has_layer_change)
    {
        m_layers = entry.layers_before;
        m_current_layer_name = entry.current_layer_before;
    }
    if (entry.has_drawing_settings_change)
    {
        m_drawing_settings = entry.drawing_settings_before;
    }
    if (entry.has_text_style_change)
    {
        m_text_styles = entry.text_styles_before;
        m_current_text_style_name = entry.current_text_style_before;
    }
    if (entry.has_dimension_style_change)
    {
        m_dimension_styles = entry.dimension_styles_before;
        m_current_dimension_style_name = entry.current_dimension_style_before;
    }
    const bool layer_change = entry.has_layer_change;
    const bool drawing_change = entry.has_drawing_settings_change;
    const bool text_change = entry.has_text_style_change;
    const bool dimension_change = entry.has_dimension_style_change;
    m_redo_stack.push_back(std::move(entry));
    setModified(true);
    if (layer_change)
    {
        emit layersChanged();
        emit currentLayerChanged(m_current_layer_name);
    }
    if (drawing_change)
    {
        emit drawingSettingsChanged();
    }
    if (text_change)
    {
        emit textStylesChanged();
        emit currentTextStyleChanged(m_current_text_style_name);
    }
    if (dimension_change)
    {
        emit dimensionStylesChanged();
        emit currentDimensionStyleChanged(m_current_dimension_style_name);
    }
    emitDocumentState();
}

void VpCadDocument::redo()
{
    if (m_redo_stack.empty())
    {
        return;
    }
    VpHistoryEntry entry = std::move(m_redo_stack.back());
    m_redo_stack.pop_back();
    eraseRecordsById(m_entities, entry.removed_entities);
    m_entities.insert(m_entities.end(), entry.added_entities.begin(), entry.added_entities.end());
    if (entry.has_entity_order_change)
    {
        applyEntityOrder(m_entities, entry.entity_order_after);
    }
    if (entry.has_layer_change)
    {
        m_layers = entry.layers_after;
        m_current_layer_name = entry.current_layer_after;
    }
    if (entry.has_drawing_settings_change)
    {
        m_drawing_settings = entry.drawing_settings_after;
    }
    if (entry.has_text_style_change)
    {
        m_text_styles = entry.text_styles_after;
        m_current_text_style_name = entry.current_text_style_after;
    }
    if (entry.has_dimension_style_change)
    {
        m_dimension_styles = entry.dimension_styles_after;
        m_current_dimension_style_name = entry.current_dimension_style_after;
    }
    const bool layer_change = entry.has_layer_change;
    const bool drawing_change = entry.has_drawing_settings_change;
    const bool text_change = entry.has_text_style_change;
    const bool dimension_change = entry.has_dimension_style_change;
    m_undo_stack.push_back(std::move(entry));
    setModified(true);
    if (layer_change)
    {
        emit layersChanged();
        emit currentLayerChanged(m_current_layer_name);
    }
    if (drawing_change)
    {
        emit drawingSettingsChanged();
    }
    if (text_change)
    {
        emit textStylesChanged();
        emit currentTextStyleChanged(m_current_text_style_name);
    }
    if (dimension_change)
    {
        emit dimensionStylesChanged();
        emit currentDimensionStyleChanged(m_current_dimension_style_name);
    }
    emitDocumentState();
}

void VpCadDocument::commitEntities(QString label, std::vector<VpEntityRecord> added_entities,
                                   std::vector<VpEntityRecord> removed_entities,
                                   std::optional<VpDrawingSettings> drawing_settings,
                                   std::optional<std::vector<VpEntityId>> entity_order)
{
    VpHistoryEntry entry;
    entry.label = std::move(label);
    entry.has_entity_order_change =
        entity_order && (entity_order->size() != m_entities.size() ||
                         !std::equal(entity_order->begin(), entity_order->end(), m_entities.begin(),
                                     [](VpEntityId entity_id, const VpEntityRecord& entity)
                                     {
                                         return entity_id == entity.id;
                                     }));
    if (entry.has_entity_order_change)
    {
        entry.entity_order_before = entityOrder(m_entities);
    }
    eraseRecordsById(m_entities, removed_entities);
    m_entities.insert(m_entities.end(), added_entities.begin(), added_entities.end());
    if (entry.has_entity_order_change)
    {
        applyEntityOrder(m_entities, *entity_order);
        entry.entity_order_after = std::move(*entity_order);
    }
    entry.added_entities = std::move(added_entities);
    entry.removed_entities = std::move(removed_entities);
    if (drawing_settings && *drawing_settings != m_drawing_settings)
    {
        entry.has_drawing_settings_change = true;
        entry.drawing_settings_before = m_drawing_settings;
        entry.drawing_settings_after = *drawing_settings;
        m_drawing_settings = *drawing_settings;
    }
    const bool drawing_change = entry.has_drawing_settings_change;
    m_undo_stack.push_back(std::move(entry));
    m_redo_stack.clear();
    setModified(true);
    if (drawing_change)
    {
        emit drawingSettingsChanged();
    }
    emitDocumentState();
}

} // namespace Vp
