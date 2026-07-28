#include "s_cad_document.h"

#include <algorithm>
#include <utility>

namespace smartGraphics
{

QStringList SCadDocument::layerStateNames() const
{
    QStringList state_names;
    state_names.reserve(static_cast<int>(m_layer_states.size()));
    for (const SLayerStateRecord& state : m_layer_states)
    {
        state_names.append(state.name);
    }
    return state_names;
}

bool SCadDocument::saveLayerState(const QString& state_name)
{
    const QString normalized_name = state_name.trimmed();
    if (normalized_name.isEmpty())
    {
        return false;
    }
    auto iterator =
        std::find_if(m_layer_states.begin(), m_layer_states.end(),
                     [&](const SLayerStateRecord& state)
                     {
                         return state.name.compare(normalized_name, Qt::CaseInsensitive) == 0;
                     });
    SLayerStateRecord snapshot{normalized_name, m_layers, m_current_layer_name};
    if (iterator == m_layer_states.end())
    {
        m_layer_states.push_back(std::move(snapshot));
    }
    else
    {
        *iterator = std::move(snapshot);
    }
    setModified(true);
    emit layerStatesChanged();
    emit documentChanged();
    return true;
}

bool SCadDocument::restoreLayerState(const QString& state_name)
{
    const auto state_iterator =
        std::find_if(m_layer_states.begin(), m_layer_states.end(),
                     [&](const SLayerStateRecord& state)
                     {
                         return state.name.compare(state_name.trimmed(), Qt::CaseInsensitive) == 0;
                     });
    if (state_iterator == m_layer_states.end())
    {
        return false;
    }
    const std::vector<SLayerRecord> layers_before = m_layers;
    const QString current_layer_before = m_current_layer_name;
    bool has_change = false;
    for (SLayerRecord& layer_record : m_layers)
    {
        const auto snapshot_iterator = std::find_if(
            state_iterator->layers.begin(), state_iterator->layers.end(),
            [&](const SLayerRecord& snapshot)
            {
                return snapshot.id == layer_record.id ||
                       snapshot.name.compare(layer_record.name, Qt::CaseInsensitive) == 0;
            });
        if (snapshot_iterator == state_iterator->layers.end())
        {
            continue;
        }
        SLayerRecord restored = *snapshot_iterator;
        restored.id = layer_record.id;
        restored.name = layer_record.name;
        if (restored.color != layer_record.color ||
            restored.line_width_mm != layer_record.line_width_mm ||
            restored.is_visible != layer_record.is_visible ||
            restored.is_locked != layer_record.is_locked ||
            restored.is_plottable != layer_record.is_plottable ||
            restored.is_frozen != layer_record.is_frozen ||
            restored.line_type != layer_record.line_type ||
            restored.transparency != layer_record.transparency)
        {
            layer_record = std::move(restored);
            has_change = true;
        }
    }
    const SLayerRecord* restored_current = layer(state_iterator->current_layer_name);
    if (restored_current && !restored_current->is_frozen &&
        m_current_layer_name != restored_current->name)
    {
        m_current_layer_name = restored_current->name;
        has_change = true;
    }
    if (!has_change)
    {
        return true;
    }
    commitLayerChange(tr("恢复图层状态"), layers_before, current_layer_before, {}, {});
    return true;
}

bool SCadDocument::removeLayerState(const QString& state_name)
{
    const auto iterator =
        std::find_if(m_layer_states.begin(), m_layer_states.end(),
                     [&](const SLayerStateRecord& state)
                     {
                         return state.name.compare(state_name.trimmed(), Qt::CaseInsensitive) == 0;
                     });
    if (iterator == m_layer_states.end())
    {
        return false;
    }
    m_layer_states.erase(iterator);
    setModified(true);
    emit layerStatesChanged();
    emit documentChanged();
    return true;
}

} // namespace smartGraphics
