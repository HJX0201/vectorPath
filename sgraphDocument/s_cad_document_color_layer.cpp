#include "s_cad_document.h"

#include <QHash>
#include <QSet>
#include <algorithm>

namespace vectorPath
{
namespace
{

bool isProtectedLayerName(const QString& layer_name)
{
    return layer_name.compare(QStringLiteral("0"), Qt::CaseInsensitive) == 0 ||
           layer_name.compare(QStringLiteral("DEFPOINTS"), Qt::CaseInsensitive) == 0;
}

QString uniqueColorLayerName(const SCadDocument& document, const QColor& color)
{
    const QString prefix =
        QStringLiteral("颜色_%1").arg(color.name(QColor::HexRgb).mid(1).toUpper());
    for (int suffix = 1; suffix < 10000; ++suffix)
    {
        const QString candidate =
            QStringLiteral("%1_%2").arg(prefix).arg(suffix, 3, 10, QLatin1Char('0'));
        if (!document.layer(candidate))
        {
            return candidate;
        }
    }
    return {};
}

} // namespace

void SCadDocument::setPreferredDrawingColor(const QColor& color)
{
    m_preferred_drawing_color = color.isValid() ? color : QColor{};
}

QColor SCadDocument::preferredDrawingColor() const
{
    return m_preferred_drawing_color;
}

QString SCadDocument::resolveDrawingLayerName()
{
    if (!m_preferred_drawing_color.isValid())
    {
        return m_current_layer_name;
    }
    const SLayerRecord* current_layer = layer(m_current_layer_name);
    if (current_layer && !current_layer->is_frozen &&
        current_layer->color.rgb() == m_preferred_drawing_color.rgb())
    {
        return current_layer->name;
    }
    const auto matching_layer =
        std::find_if(m_layers.begin(), m_layers.end(),
                     [this](const SLayerRecord& layer_record)
                     {
                         return !layer_record.is_frozen &&
                                layer_record.name.compare(QStringLiteral("DEFPOINTS"),
                                                          Qt::CaseInsensitive) != 0 &&
                                layer_record.color.rgb() == m_preferred_drawing_color.rgb();
                     });
    if (matching_layer != m_layers.end())
    {
        m_current_layer_name = matching_layer->name;
        emit currentLayerChanged(m_current_layer_name);
        return m_current_layer_name;
    }

    const QString layer_name = uniqueColorLayerName(*this, m_preferred_drawing_color);
    if (layer_name.isEmpty())
    {
        return m_current_layer_name;
    }
    m_layers.push_back({reserveLayerId(), layer_name, m_preferred_drawing_color, 0.25, true, false,
                        true, false, QStringLiteral("Continuous"), 0});
    m_current_layer_name = layer_name;
    setModified(true);
    emit layersChanged();
    emit currentLayerChanged(m_current_layer_name);
    emit documentChanged();
    return m_current_layer_name;
}

QString SCadDocument::assignEntitiesToColorLayer(const std::vector<SEntityId>& entity_ids,
                                                 const QColor& color)
{
    if (entity_ids.empty() || !color.isValid())
    {
        return {};
    }

    QSet<SEntityId> requested_ids;
    for (SEntityId entity_id : entity_ids)
    {
        requested_ids.insert(entity_id);
    }
    std::vector<SEntityRecord> removed_entities;
    std::vector<SEntityRecord> added_entities;
    QSet<QString> source_layer_names;
    QSet<SEntityId> moved_ids;
    const SLayerRecord* template_layer = nullptr;
    for (const SEntityRecord& entity : m_entities)
    {
        if (!requested_ids.contains(entity.id) || isLayerLocked(entity.layer_name))
        {
            continue;
        }
        removed_entities.push_back(entity);
        moved_ids.insert(entity.id);
        source_layer_names.insert(entity.layer_name);
        if (!template_layer)
        {
            template_layer = layer(entity.layer_name);
        }
    }
    const QString new_layer_name = uniqueColorLayerName(*this, color);
    if (removed_entities.empty() || new_layer_name.isEmpty())
    {
        return {};
    }

    const std::vector<SLayerRecord> layers_before = m_layers;
    const QString current_layer_before = m_current_layer_name;
    SLayerRecord new_layer = template_layer ? *template_layer : SLayerRecord{};
    new_layer.id = reserveLayerId();
    new_layer.name = new_layer_name;
    new_layer.color = color;
    new_layer.is_visible = true;
    new_layer.is_locked = false;
    new_layer.is_frozen = false;
    new_layer.is_plottable = true;
    m_layers.push_back(new_layer);

    added_entities.reserve(removed_entities.size());
    for (const SEntityRecord& entity : removed_entities)
    {
        SEntityRecord replacement = entity;
        replacement.layer_name = new_layer_name;
        added_entities.push_back(std::move(replacement));
    }

    for (const QString& source_layer_name : source_layer_names)
    {
        if (isProtectedLayerName(source_layer_name))
        {
            continue;
        }
        const bool has_unselected_entity =
            std::any_of(m_entities.begin(), m_entities.end(),
                        [&moved_ids, &source_layer_name](const SEntityRecord& entity)
                        {
                            return !moved_ids.contains(entity.id) &&
                                   entity.layer_name.compare(source_layer_name,
                                                             Qt::CaseInsensitive) == 0;
                        });
        if (has_unselected_entity)
        {
            continue;
        }
        if (m_current_layer_name.compare(source_layer_name, Qt::CaseInsensitive) == 0)
        {
            m_current_layer_name = new_layer_name;
        }
        m_layers.erase(std::remove_if(m_layers.begin(), m_layers.end(),
                                      [&source_layer_name](const SLayerRecord& layer_record)
                                      {
                                          return layer_record.name.compare(
                                                     source_layer_name,
                                                     Qt::CaseInsensitive) == 0;
                                      }),
                       m_layers.end());
    }

    commitLayerChange(tr("修改实体颜色"), layers_before, current_layer_before,
                      std::move(added_entities), std::move(removed_entities));
    return new_layer_name;
}

int SCadDocument::mergeLayersByColor()
{
    QHash<QRgb, QString> target_names;
    QHash<QString, QString> source_to_target;
    for (const SLayerRecord& layer_record : m_layers)
    {
        if (layer_record.name.compare(QStringLiteral("DEFPOINTS"), Qt::CaseInsensitive) == 0)
        {
            continue;
        }
        const QRgb color_key = layer_record.color.rgba();
        const auto target_iterator = target_names.constFind(color_key);
        if (target_iterator == target_names.constEnd())
        {
            target_names.insert(color_key, layer_record.name);
        }
        else if (layer_record.name.compare(QStringLiteral("0"), Qt::CaseInsensitive) == 0)
        {
            const QString former_target = *target_iterator;
            for (auto iterator = source_to_target.begin(); iterator != source_to_target.end();
                 ++iterator)
            {
                if (iterator.value() == former_target)
                {
                    iterator.value() = layer_record.name;
                }
            }
            source_to_target.insert(former_target, layer_record.name);
            target_names[color_key] = layer_record.name;
        }
        else
        {
            source_to_target.insert(layer_record.name, *target_iterator);
        }
    }
    if (source_to_target.isEmpty())
    {
        return 0;
    }

    const std::vector<SLayerRecord> layers_before = m_layers;
    const QString current_layer_before = m_current_layer_name;
    std::vector<SEntityRecord> removed_entities;
    std::vector<SEntityRecord> added_entities;
    for (const SEntityRecord& entity : m_entities)
    {
        const auto target_iterator = source_to_target.constFind(entity.layer_name);
        if (target_iterator == source_to_target.constEnd())
        {
            continue;
        }
        removed_entities.push_back(entity);
        SEntityRecord replacement = entity;
        replacement.layer_name = *target_iterator;
        added_entities.push_back(std::move(replacement));
    }
    for (auto iterator = source_to_target.constBegin(); iterator != source_to_target.constEnd();
         ++iterator)
    {
        if (m_current_layer_name.compare(iterator.key(), Qt::CaseInsensitive) == 0)
        {
            m_current_layer_name = iterator.value();
        }
    }
    m_layers.erase(std::remove_if(m_layers.begin(), m_layers.end(),
                                  [&source_to_target](const SLayerRecord& layer_record)
                                  {
                                      return source_to_target.contains(layer_record.name);
                                  }),
                   m_layers.end());

    const int merged_count = source_to_target.size();
    commitLayerChange(tr("合并同色图层"), layers_before, current_layer_before,
                      std::move(added_entities), std::move(removed_entities));
    return merged_count;
}

} // namespace vectorPath
