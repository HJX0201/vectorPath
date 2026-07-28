#include "s_cad_document.h"

#include <algorithm>
#include <unordered_map>
#include <utility>

namespace smartGraphics
{
namespace
{

void eraseEntities(std::vector<SEntityRecord>& entities,
                   const std::vector<SEntityRecord>& entities_to_erase)
{
    for (const SEntityRecord& entity : entities_to_erase)
    {
        entities.erase(std::remove_if(entities.begin(), entities.end(),
                                      [entity](const SEntityRecord& candidate)
                                      {
                                          return candidate.id == entity.id;
                                      }),
                       entities.end());
    }
}

std::vector<SEntityId> entityOrder(const std::vector<SEntityRecord>& entities)
{
    std::vector<SEntityId> order;
    order.reserve(entities.size());
    for (const SEntityRecord& entity : entities)
    {
        order.push_back(entity.id);
    }
    return order;
}

void applyEntityOrder(std::vector<SEntityRecord>& entities,
                      const std::vector<SEntityId>& order)
{
    std::unordered_map<SEntityId, std::size_t> ranks;
    ranks.reserve(order.size());
    for (std::size_t index = 0; index < order.size(); ++index)
    {
        ranks.emplace(order[index], index);
    }
    std::stable_sort(entities.begin(), entities.end(),
                     [&ranks](const SEntityRecord& first, const SEntityRecord& second)
                     {
                         return ranks.at(first.id) < ranks.at(second.id);
                     });
}

} // namespace

void SCadDocument::undo()
{
    if (m_undo_stack.empty())
    {
        return;
    }
    SHistoryEntry entry = std::move(m_undo_stack.back());
    m_undo_stack.pop_back();
    eraseEntities(m_entities, entry.added_entities);
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

void SCadDocument::redo()
{
    if (m_redo_stack.empty())
    {
        return;
    }
    SHistoryEntry entry = std::move(m_redo_stack.back());
    m_redo_stack.pop_back();
    eraseEntities(m_entities, entry.removed_entities);
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

void SCadDocument::commitEntities(QString label, std::vector<SEntityRecord> added_entities,
                                  std::vector<SEntityRecord> removed_entities,
                                  std::optional<SDrawingSettings> drawing_settings,
                                  std::optional<std::vector<SEntityId>> entity_order)
{
    SHistoryEntry entry;
    entry.label = std::move(label);
    entry.entity_order_before = entityOrder(m_entities);
    eraseEntities(m_entities, removed_entities);
    m_entities.insert(m_entities.end(), added_entities.begin(), added_entities.end());
    if (entity_order && *entity_order != entry.entity_order_before)
    {
        applyEntityOrder(m_entities, *entity_order);
        entry.has_entity_order_change = true;
        entry.entity_order_after = *entity_order;
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

} // namespace smartGraphics
