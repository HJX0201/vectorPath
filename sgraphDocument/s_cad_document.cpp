#include "s_cad_document.h"

#include "s_document_transaction.h"

#include <QFileInfo>
#include <algorithm>
#include <cmath>

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

} // namespace

SCadDocument::SCadDocument(QObject* parent) : QObject(parent)
{
}

const std::vector<SEntityRecord>& SCadDocument::entities() const noexcept
{
    return m_entities;
}

QString SCadDocument::filePath() const
{
    return m_file_path;
}

QString SCadDocument::displayName() const
{
    return m_file_path.isEmpty() ? tr("未命名") : QFileInfo(m_file_path).fileName();
}

bool SCadDocument::isModified() const noexcept
{
    return m_is_modified;
}

bool SCadDocument::canUndo() const noexcept
{
    return !m_undo_stack.empty();
}

bool SCadDocument::canRedo() const noexcept
{
    return !m_redo_stack.empty();
}

const SDrawingSettings& SCadDocument::drawingSettings() const noexcept
{
    return m_drawing_settings;
}

QStringList SCadDocument::layerNames() const
{
    QStringList layer_names;
    layer_names.reserve(static_cast<int>(m_layers.size()));
    for (const SLayerRecord& layer_record : m_layers)
    {
        layer_names.append(layer_record.name);
    }
    return layer_names;
}

const std::vector<SLayerRecord>& SCadDocument::layers() const noexcept
{
    return m_layers;
}

const SLayerRecord* SCadDocument::layer(const QString& layer_name) const noexcept
{
    const auto iterator =
        std::find_if(m_layers.begin(), m_layers.end(),
                     [&layer_name](const SLayerRecord& layer_record)
                     {
                         return layer_record.name.compare(layer_name, Qt::CaseInsensitive) == 0;
                     });
    return iterator == m_layers.end() ? nullptr : &*iterator;
}

QString SCadDocument::currentLayerName() const
{
    return m_current_layer_name;
}

bool SCadDocument::addLayer(const QString& layer_name)
{
    const QString normalized_name = layer_name.trimmed();
    if (normalized_name.isEmpty() || layer(normalized_name))
    {
        return false;
    }
    static const QColor kLayerColors[] = {
        QColor(94, 201, 255),  QColor(255, 194, 92),  QColor(105, 219, 142),
        QColor(222, 122, 220), QColor(255, 112, 112), QColor(120, 145, 255),
    };
    const bool is_defpoints =
        normalized_name.compare(QStringLiteral("DEFPOINTS"), Qt::CaseInsensitive) == 0;
    const std::size_t color_index =
        (m_layers.size() - 1U) % (sizeof(kLayerColors) / sizeof(kLayerColors[0]));
    m_layers.push_back({reserveLayerId(), normalized_name,
                        is_defpoints ? QColor(128, 138, 148) : kLayerColors[color_index], 0.25,
                        true, false, !is_defpoints});
    setModified(true);
    emit layersChanged();
    emit documentChanged();
    return true;
}

bool SCadDocument::removeLayer(const QString& layer_name)
{
    if (layer_name.compare(QStringLiteral("0"), Qt::CaseInsensitive) == 0 ||
        layer_name.compare(QStringLiteral("DEFPOINTS"), Qt::CaseInsensitive) == 0 ||
        layer_name.compare(m_current_layer_name, Qt::CaseInsensitive) == 0)
    {
        return false;
    }
    const bool is_in_use =
        std::any_of(m_entities.begin(), m_entities.end(),
                    [&layer_name](const SEntityRecord& entity)
                    {
                        return entity.layer_name.compare(layer_name, Qt::CaseInsensitive) == 0;
                    });
    if (is_in_use)
    {
        return false;
    }
    const auto iterator =
        std::find_if(m_layers.begin(), m_layers.end(),
                     [&layer_name](const SLayerRecord& layer_record)
                     {
                         return layer_record.name.compare(layer_name, Qt::CaseInsensitive) == 0;
                     });
    if (iterator == m_layers.end())
    {
        return false;
    }
    m_layers.erase(iterator);
    setModified(true);
    emit layersChanged();
    emit documentChanged();
    return true;
}

bool SCadDocument::renameLayer(const QString& layer_name, const QString& new_name)
{
    const QString normalized_name = new_name.trimmed();
    SLayerRecord* source_layer = mutableLayer(layer_name);
    if (!source_layer || normalized_name.isEmpty() || layer(normalized_name) ||
        source_layer->name.compare(QStringLiteral("0"), Qt::CaseInsensitive) == 0 ||
        source_layer->name.compare(QStringLiteral("DEFPOINTS"), Qt::CaseInsensitive) == 0)
    {
        return false;
    }
    const std::vector<SLayerRecord> layers_before = m_layers;
    const QString current_layer_before = m_current_layer_name;
    const QString source_name = source_layer->name;
    std::vector<SEntityRecord> removed_entities;
    std::vector<SEntityRecord> added_entities;
    for (const SEntityRecord& entity : m_entities)
    {
        if (entity.layer_name.compare(source_name, Qt::CaseInsensitive) == 0)
        {
            removed_entities.push_back(entity);
            SEntityRecord replacement = entity;
            replacement.layer_name = normalized_name;
            added_entities.push_back(std::move(replacement));
        }
    }
    source_layer->name = normalized_name;
    if (m_current_layer_name.compare(source_name, Qt::CaseInsensitive) == 0)
    {
        m_current_layer_name = normalized_name;
    }
    commitLayerChange(tr("重命名图层"), layers_before, current_layer_before,
                      std::move(added_entities), std::move(removed_entities));
    return true;
}

bool SCadDocument::mergeLayer(const QString& source_layer_name, const QString& target_layer_name)
{
    const SLayerRecord* source_layer = layer(source_layer_name);
    const SLayerRecord* target_layer = layer(target_layer_name);
    if (!source_layer || !target_layer || source_layer->id == target_layer->id ||
        source_layer->name.compare(QStringLiteral("0"), Qt::CaseInsensitive) == 0 ||
        source_layer->name.compare(QStringLiteral("DEFPOINTS"), Qt::CaseInsensitive) == 0)
    {
        return false;
    }
    const std::vector<SLayerRecord> layers_before = m_layers;
    const QString current_layer_before = m_current_layer_name;
    const QString source_name = source_layer->name;
    const QString target_name = target_layer->name;
    const SLayerId source_id = source_layer->id;
    std::vector<SEntityRecord> removed_entities;
    std::vector<SEntityRecord> added_entities;
    for (const SEntityRecord& entity : m_entities)
    {
        if (entity.layer_name.compare(source_name, Qt::CaseInsensitive) == 0)
        {
            removed_entities.push_back(entity);
            SEntityRecord replacement = entity;
            replacement.layer_name = target_name;
            added_entities.push_back(std::move(replacement));
        }
    }
    m_layers.erase(std::remove_if(m_layers.begin(), m_layers.end(),
                                  [source_id](const SLayerRecord& record)
                                  {
                                      return record.id == source_id;
                                  }),
                   m_layers.end());
    if (m_current_layer_name.compare(source_name, Qt::CaseInsensitive) == 0)
    {
        m_current_layer_name = target_name;
    }
    commitLayerChange(tr("合并图层"), layers_before, current_layer_before,
                      std::move(added_entities), std::move(removed_entities));
    return true;
}

bool SCadDocument::removeLayerWithContents(const QString& layer_name)
{
    const SLayerRecord* source_layer = layer(layer_name);
    if (!source_layer ||
        source_layer->name.compare(QStringLiteral("0"), Qt::CaseInsensitive) == 0 ||
        source_layer->name.compare(QStringLiteral("DEFPOINTS"), Qt::CaseInsensitive) == 0)
    {
        return false;
    }
    const std::vector<SLayerRecord> layers_before = m_layers;
    const QString current_layer_before = m_current_layer_name;
    const QString source_name = source_layer->name;
    const SLayerId source_id = source_layer->id;
    std::vector<SEntityRecord> removed_entities;
    for (const SEntityRecord& entity : m_entities)
    {
        if (entity.layer_name.compare(source_name, Qt::CaseInsensitive) == 0)
        {
            removed_entities.push_back(entity);
        }
    }
    m_layers.erase(std::remove_if(m_layers.begin(), m_layers.end(),
                                  [source_id](const SLayerRecord& record)
                                  {
                                      return record.id == source_id;
                                  }),
                   m_layers.end());
    if (m_current_layer_name.compare(source_name, Qt::CaseInsensitive) == 0)
    {
        m_current_layer_name = QStringLiteral("0");
    }
    commitLayerChange(tr("连同内容删除图层"), layers_before, current_layer_before, {},
                      std::move(removed_entities));
    return true;
}

bool SCadDocument::setCurrentLayer(const QString& layer_name)
{
    const SLayerRecord* layer_record = layer(layer_name);
    if (!layer_record || layer_record->is_frozen || m_current_layer_name == layer_record->name)
    {
        return false;
    }
    m_current_layer_name = layer_record->name;
    emit currentLayerChanged(m_current_layer_name);
    return true;
}

QColor SCadDocument::layerColor(const QString& layer_name) const
{
    const SLayerRecord* layer_record = layer(layer_name);
    return layer_record ? layer_record->color : QColor(220, 228, 238);
}

double SCadDocument::layerLineWidth(const QString& layer_name) const noexcept
{
    const SLayerRecord* layer_record = layer(layer_name);
    return layer_record ? layer_record->line_width_mm : 0.25;
}

QString SCadDocument::layerLineType(const QString& layer_name) const
{
    const SLayerRecord* layer_record = layer(layer_name);
    return layer_record ? layer_record->line_type : QStringLiteral("Continuous");
}

int SCadDocument::layerTransparency(const QString& layer_name) const noexcept
{
    const SLayerRecord* layer_record = layer(layer_name);
    return layer_record ? layer_record->transparency : 0;
}

bool SCadDocument::isLayerVisible(const QString& layer_name) const noexcept
{
    const SLayerRecord* layer_record = layer(layer_name);
    return !layer_record || (layer_record->is_visible && !layer_record->is_frozen);
}

bool SCadDocument::isLayerLocked(const QString& layer_name) const noexcept
{
    const SLayerRecord* layer_record = layer(layer_name);
    return layer_record && layer_record->is_locked;
}

bool SCadDocument::isLayerFrozen(const QString& layer_name) const noexcept
{
    const SLayerRecord* layer_record = layer(layer_name);
    return layer_record && layer_record->is_frozen;
}

bool SCadDocument::setLayerVisible(const QString& layer_name, bool is_visible)
{
    SLayerRecord* layer_record = mutableLayer(layer_name);
    if (!layer_record || layer_record->is_visible == is_visible)
    {
        return false;
    }
    const std::vector<SLayerRecord> layers_before = m_layers;
    const QString current_layer_before = m_current_layer_name;
    layer_record->is_visible = is_visible;
    commitLayerChange(tr("更改图层可见性"), layers_before, current_layer_before, {}, {});
    return true;
}

bool SCadDocument::setLayerLocked(const QString& layer_name, bool is_locked)
{
    SLayerRecord* layer_record = mutableLayer(layer_name);
    if (!layer_record || layer_record->is_locked == is_locked)
    {
        return false;
    }
    const std::vector<SLayerRecord> layers_before = m_layers;
    const QString current_layer_before = m_current_layer_name;
    layer_record->is_locked = is_locked;
    commitLayerChange(tr("更改图层锁定"), layers_before, current_layer_before, {}, {});
    return true;
}

bool SCadDocument::setLayerFrozen(const QString& layer_name, bool is_frozen)
{
    SLayerRecord* layer_record = mutableLayer(layer_name);
    if (!layer_record || layer_record->is_frozen == is_frozen ||
        (is_frozen && layer_record->name == m_current_layer_name))
    {
        return false;
    }
    const std::vector<SLayerRecord> layers_before = m_layers;
    const QString current_layer_before = m_current_layer_name;
    layer_record->is_frozen = is_frozen;
    commitLayerChange(tr("冻结或解冻图层"), layers_before, current_layer_before, {}, {});
    return true;
}

bool SCadDocument::isolateLayer(const QString& layer_name)
{
    const SLayerRecord* target_layer = layer(layer_name);
    if (!target_layer)
    {
        return false;
    }
    const std::vector<SLayerRecord> layers_before = m_layers;
    const QString current_layer_before = m_current_layer_name;
    bool has_change = false;
    for (SLayerRecord& layer_record : m_layers)
    {
        const bool should_be_visible = layer_record.id == target_layer->id;
        if (layer_record.is_visible != should_be_visible)
        {
            layer_record.is_visible = should_be_visible;
            has_change = true;
        }
    }
    if (!has_change)
    {
        return false;
    }
    commitLayerChange(tr("隔离图层"), layers_before, current_layer_before, {}, {});
    return true;
}

bool SCadDocument::setLayerPlottable(const QString& layer_name, bool is_plottable)
{
    SLayerRecord* layer_record = mutableLayer(layer_name);
    if (!layer_record || layer_record->is_plottable == is_plottable)
    {
        return false;
    }
    const std::vector<SLayerRecord> layers_before = m_layers;
    const QString current_layer_before = m_current_layer_name;
    layer_record->is_plottable = is_plottable;
    commitLayerChange(tr("更改图层打印状态"), layers_before, current_layer_before, {}, {});
    return true;
}

bool SCadDocument::setLayerColor(const QString& layer_name, const QColor& color)
{
    SLayerRecord* layer_record = mutableLayer(layer_name);
    if (!layer_record || !color.isValid() || layer_record->color == color)
    {
        return false;
    }
    const std::vector<SLayerRecord> layers_before = m_layers;
    const QString current_layer_before = m_current_layer_name;
    layer_record->color = color;
    commitLayerChange(tr("更改图层颜色"), layers_before, current_layer_before, {}, {});
    return true;
}

bool SCadDocument::previewLayerColor(const QString& layer_name, const QColor& color)
{
    SLayerRecord* layer_record = mutableLayer(layer_name);
    if (!layer_record || !color.isValid() || layer_record->color == color)
    {
        return false;
    }
    layer_record->color = color;
    emit layersChanged();
    emit documentChanged();
    return true;
}

bool SCadDocument::setLayerLineWidth(const QString& layer_name, double line_width_mm)
{
    SLayerRecord* layer_record = mutableLayer(layer_name);
    if (!layer_record || line_width_mm < 0.0 || line_width_mm > 2.11 ||
        layer_record->line_width_mm == line_width_mm)
    {
        return false;
    }
    const std::vector<SLayerRecord> layers_before = m_layers;
    const QString current_layer_before = m_current_layer_name;
    layer_record->line_width_mm = line_width_mm;
    commitLayerChange(tr("更改图层线宽"), layers_before, current_layer_before, {}, {});
    return true;
}

bool SCadDocument::setLayerLineType(const QString& layer_name, const QString& line_type)
{
    static const QStringList kLineTypes{QStringLiteral("Continuous"), QStringLiteral("Dashed"),
                                        QStringLiteral("Dotted"),     QStringLiteral("DashDot"),
                                        QStringLiteral("Center"),     QStringLiteral("Hidden")};
    const auto type_iterator =
        std::find_if(kLineTypes.begin(), kLineTypes.end(),
                     [&](const QString& candidate)
                     {
                         return candidate.compare(line_type.trimmed(), Qt::CaseInsensitive) == 0;
                     });
    SLayerRecord* layer_record = mutableLayer(layer_name);
    if (!layer_record || type_iterator == kLineTypes.end() ||
        layer_record->line_type == *type_iterator)
    {
        return false;
    }
    const std::vector<SLayerRecord> layers_before = m_layers;
    const QString current_layer_before = m_current_layer_name;
    layer_record->line_type = *type_iterator;
    commitLayerChange(tr("更改图层线型"), layers_before, current_layer_before, {}, {});
    return true;
}

bool SCadDocument::setLayerTransparency(const QString& layer_name, int transparency)
{
    SLayerRecord* layer_record = mutableLayer(layer_name);
    if (!layer_record || transparency < 0 || transparency > 90 ||
        layer_record->transparency == transparency)
    {
        return false;
    }
    const std::vector<SLayerRecord> layers_before = m_layers;
    const QString current_layer_before = m_current_layer_name;
    layer_record->transparency = transparency;
    commitLayerChange(tr("更改图层透明度"), layers_before, current_layer_before, {}, {});
    return true;
}

std::unique_ptr<SDocumentTransaction> SCadDocument::beginTransaction(const QString& label)
{
    return std::make_unique<SDocumentTransaction>(*this, label);
}

void SCadDocument::clear()
{
    m_entities.clear();
    m_undo_stack.clear();
    m_redo_stack.clear();
    m_next_entity_id = 1;
    m_next_layer_id = 1;
    m_layers = {
        {0, QStringLiteral("0"), QColor(220, 228, 238), 0.25, true, false, true},
    };
    m_layer_states.clear();
    m_current_layer_name = QStringLiteral("0");
    m_drawing_settings = {};
    m_text_styles = {{}};
    m_current_text_style_name = QStringLiteral("Standard");
    m_dimension_styles = {{}};
    m_current_dimension_style_name = QStringLiteral("Standard");
    m_file_path.clear();
    setModified(false);
    emit filePathChanged({});
    emit layersChanged();
    emit currentLayerChanged(m_current_layer_name);
    emit layerStatesChanged();
    emit drawingSettingsChanged();
    emit textStylesChanged();
    emit currentTextStyleChanged(m_current_text_style_name);
    emit dimensionStylesChanged();
    emit currentDimensionStyleChanged(m_current_dimension_style_name);
    emitDocumentState();
}

SEntityId SCadDocument::reserveEntityId() noexcept
{
    return m_next_entity_id++;
}

SLayerId SCadDocument::reserveLayerId() noexcept
{
    return m_next_layer_id++;
}

SLayerRecord* SCadDocument::mutableLayer(const QString& layer_name) noexcept
{
    const auto iterator =
        std::find_if(m_layers.begin(), m_layers.end(),
                     [&layer_name](const SLayerRecord& layer_record)
                     {
                         return layer_record.name.compare(layer_name, Qt::CaseInsensitive) == 0;
                     });
    return iterator == m_layers.end() ? nullptr : &*iterator;
}

void SCadDocument::ensureDefpointsLayer()
{
    if (layer(QStringLiteral("DEFPOINTS")))
    {
        return;
    }
    m_layers.push_back({reserveLayerId(), QStringLiteral("DEFPOINTS"), QColor(128, 138, 148),
                        0.25, true, false, false});
    emit layersChanged();
}

void SCadDocument::commitLayerChange(QString label, std::vector<SLayerRecord> layers_before,
                                     QString current_layer_before,
                                     std::vector<SEntityRecord> added_entities,
                                     std::vector<SEntityRecord> removed_entities)
{
    eraseEntities(m_entities, removed_entities);
    m_entities.insert(m_entities.end(), added_entities.begin(), added_entities.end());
    SHistoryEntry entry;
    entry.label = std::move(label);
    entry.added_entities = std::move(added_entities);
    entry.removed_entities = std::move(removed_entities);
    entry.has_layer_change = true;
    entry.layers_before = std::move(layers_before);
    entry.layers_after = m_layers;
    entry.current_layer_before = std::move(current_layer_before);
    entry.current_layer_after = m_current_layer_name;
    m_undo_stack.push_back(std::move(entry));
    m_redo_stack.clear();
    setModified(true);
    emit layersChanged();
    emit currentLayerChanged(m_current_layer_name);
    emitDocumentState();
}

void SCadDocument::setModified(bool is_modified)
{
    if (m_is_modified == is_modified)
    {
        return;
    }
    m_is_modified = is_modified;
    emit modifiedChanged(m_is_modified);
}

void SCadDocument::emitDocumentState()
{
    emit documentChanged();
    emit historyChanged(canUndo(), canRedo());
}

} // namespace smartGraphics
