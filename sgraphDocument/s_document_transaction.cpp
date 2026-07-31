#include "s_document_transaction.h"

#include "s_cad_document.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace smartCam
{

SDocumentTransaction::SDocumentTransaction(SCadDocument& document, QString label)
    : m_document(document), m_label(std::move(label))
{
}

SDocumentTransaction::~SDocumentTransaction() = default;

QString SDocumentTransaction::creationLayerName()
{
    return m_document.resolveDrawingLayerName();
}

SEntityId SDocumentTransaction::addLine(const SPoint2d& start_point, const SPoint2d& end_point)
{
    SEntityRecord entity;
    entity.id = m_document.reserveEntityId();
    entity.type = SEntityType::Line;
    entity.layer_name = creationLayerName();
    entity.geometry = SLineEntity{start_point, end_point};
    m_entities.push_back(entity);
    return entity.id;
}

SEntityId SDocumentTransaction::addCircle(const SPoint2d& center, double radius)
{
    SEntityRecord entity;
    entity.id = m_document.reserveEntityId();
    entity.type = SEntityType::Circle;
    entity.layer_name = creationLayerName();
    entity.geometry = SCircleEntity{center, radius};
    m_entities.push_back(entity);
    return entity.id;
}

SEntityId SDocumentTransaction::addArc(const SPoint2d& center, double radius, double start_angle,
                                       double end_angle)
{
    SEntityRecord entity;
    entity.id = m_document.reserveEntityId();
    entity.type = SEntityType::Arc;
    entity.layer_name = creationLayerName();
    entity.geometry = SArcEntity{center, radius, start_angle, end_angle};
    m_entities.push_back(entity);
    return entity.id;
}

SEntityId SDocumentTransaction::addEllipse(const SPoint2d& center, const SPoint2d& major_axis,
                                           const SPoint2d& minor_axis)
{
    SEntityRecord entity;
    entity.id = m_document.reserveEntityId();
    entity.type = SEntityType::Ellipse;
    entity.layer_name = creationLayerName();
    entity.geometry = SEllipseEntity{center, major_axis, minor_axis};
    m_entities.push_back(entity);
    return entity.id;
}

SEntityId SDocumentTransaction::addSpline(const std::array<SPoint2d, 4>& control_points)
{
    SEntityRecord entity;
    entity.id = m_document.reserveEntityId();
    entity.type = SEntityType::Spline;
    entity.layer_name = creationLayerName();
    entity.geometry = SSplineEntity{control_points};
    m_entities.push_back(entity);
    return entity.id;
}

SEntityId SDocumentTransaction::addPolyline(std::vector<SPoint2d> vertices, bool is_closed,
                                            std::vector<double> bulges,
                                            std::vector<double> start_widths,
                                            std::vector<double> end_widths)
{
    bulges.resize(vertices.size(), 0.0);
    start_widths.resize(vertices.size(), 0.0);
    end_widths.resize(vertices.size(), 0.0);
    SEntityRecord entity;
    entity.id = m_document.reserveEntityId();
    entity.type = SEntityType::Polyline;
    entity.layer_name = creationLayerName();
    entity.geometry = SPolylineEntity{std::move(vertices), is_closed, std::move(bulges),
                                      std::move(start_widths), std::move(end_widths)};
    m_entities.push_back(entity);
    return entity.id;
}

SEntityId SDocumentTransaction::addText(const SPoint2d& position, QString text, double height,
                                        double rotation, STextHorizontalAlignment alignment,
                                        STextVerticalAlignment vertical_alignment)
{
    SEntityRecord entity;
    entity.id = m_document.reserveEntityId();
    entity.type = SEntityType::Text;
    entity.layer_name = creationLayerName();
    entity.geometry = STextEntity{
        position,  std::move(text),   height, rotation, m_document.currentTextStyleName(),
        alignment, vertical_alignment};
    m_entities.push_back(entity);
    return entity.id;
}

SEntityId SDocumentTransaction::addMText(const SPoint2d& position, QString rich_text, double width,
                                         double height, double rotation,
                                         STextHorizontalAlignment alignment)
{
    SEntityRecord entity;
    entity.id = m_document.reserveEntityId();
    entity.type = SEntityType::MText;
    entity.layer_name = creationLayerName();
    entity.geometry = SMTextEntity{
        position, std::move(rich_text), width, height, rotation, m_document.currentTextStyleName(),
        alignment};
    m_entities.push_back(entity);
    return entity.id;
}

SEntityId SDocumentTransaction::addLeader(std::vector<SPoint2d> vertices, QString text,
                                          double text_height, double arrow_size)
{
    SEntityRecord entity;
    entity.id = m_document.reserveEntityId();
    entity.type = SEntityType::Leader;
    entity.layer_name = creationLayerName();
    entity.geometry = SLeaderEntity{std::move(vertices), std::move(text), text_height, arrow_size,
                                    m_document.currentTextStyleName()};
    m_entities.push_back(entity);
    return entity.id;
}

SEntityId SDocumentTransaction::addLinearDimension(const SPoint2d& first_point,
                                                   const SPoint2d& second_point,
                                                   const SPoint2d& dimension_line_point)
{
    SLinearDimensionEntity dimension;
    dimension.first_point = first_point;
    dimension.second_point = second_point;
    dimension.dimension_line_point = dimension_line_point;
    dimension.dimension_type = SDimensionType::Linear;
    return addDimension(std::move(dimension));
}

SEntityId SDocumentTransaction::addDimension(SLinearDimensionEntity dimension)
{
    if (dimension.style_name.trimmed().isEmpty())
    {
        dimension.style_name = m_document.currentDimensionStyleName();
    }
    SEntityRecord entity;
    entity.id = m_document.reserveEntityId();
    entity.type = SEntityType::LinearDimension;
    entity.layer_name = creationLayerName();
    entity.geometry = std::move(dimension);
    m_entities.push_back(entity);
    return entity.id;
}

SEntityId SDocumentTransaction::addHatch(std::vector<SPoint2d> boundary)
{
    return addHatch(SHatchEntity{std::move(boundary)});
}

SEntityId SDocumentTransaction::addHatch(SHatchEntity hatch)
{
    SEntityRecord entity;
    entity.id = m_document.reserveEntityId();
    entity.type = SEntityType::Hatch;
    entity.layer_name = creationLayerName();
    entity.geometry = std::move(hatch);
    m_entities.push_back(entity);
    return entity.id;
}

QString SDocumentTransaction::ensureLayer(QString preferred_name, const QColor& color,
                                          int transparency)
{
    preferred_name = preferred_name.trimmed();
    if (preferred_name.isEmpty() || !color.isValid() ||
        transparency < 0 || transparency > 90)
    {
        return {};
    }
    const auto pending_layer = [&](const QString& layer_name) -> const SLayerRecord*
    {
        const auto iterator =
            std::find_if(m_added_layers.begin(), m_added_layers.end(),
                         [&layer_name](const SLayerRecord& layer)
                         {
                             return layer.name.compare(layer_name, Qt::CaseInsensitive) == 0;
                         });
        return iterator == m_added_layers.end() ? nullptr : &*iterator;
    };
    const SLayerRecord* existing = m_document.layer(preferred_name);
    const SLayerRecord* pending = pending_layer(preferred_name);
    if ((existing && existing->color == color && existing->transparency == transparency) ||
        (pending && pending->color == color && pending->transparency == transparency))
    {
        return preferred_name;
    }
    QString layer_name = preferred_name;
    int suffix = 2;
    while (m_document.layer(layer_name) || pending_layer(layer_name))
    {
        layer_name = QStringLiteral("%1_%2").arg(preferred_name).arg(suffix++);
    }
    SLayerRecord layer;
    layer.id = m_document.reserveLayerId();
    layer.name = layer_name;
    layer.color = color;
    layer.transparency = transparency;
    m_added_layers.push_back(std::move(layer));
    return layer_name;
}

SEntityId SDocumentTransaction::addEntity(SEntityType type, SEntityGeometry geometry,
                                          QString layer_name)
{
    const bool has_pending_layer =
        std::any_of(m_added_layers.begin(), m_added_layers.end(),
                    [&layer_name](const SLayerRecord& layer)
                    {
                        return layer.name.compare(layer_name, Qt::CaseInsensitive) == 0;
                    });
    if (!m_document.layer(layer_name) && !has_pending_layer)
    {
        layer_name = creationLayerName();
    }
    SEntityRecord entity;
    entity.id = m_document.reserveEntityId();
    entity.type = type;
    entity.layer_name = std::move(layer_name);
    entity.geometry = std::move(geometry);
    m_entities.push_back(std::move(entity));
    return m_entities.back().id;
}

SEntityId SDocumentTransaction::addEntityCopy(SEntityRecord entity, bool preserve_association)
{
    entity.id = m_document.reserveEntityId();
    if (!preserve_association)
    {
        entity.associative_array.reset();
    }
    if (!m_document.layer(entity.layer_name))
    {
        entity.layer_name = m_document.currentLayerName();
    }
    m_entities.push_back(std::move(entity));
    return m_entities.back().id;
}

bool SDocumentTransaction::replaceEntity(SEntityId entity_id, SEntityRecord replacement)
{
    const auto iterator = std::find_if(m_document.m_entities.begin(), m_document.m_entities.end(),
                                       [entity_id](const SEntityRecord& entity)
                                       {
                                           return entity.id == entity_id;
                                       });
    if (iterator == m_document.m_entities.end())
    {
        return false;
    }
    m_removed_entities.push_back(*iterator);
    replacement.id = entity_id;
    m_entities.push_back(std::move(replacement));
    return true;
}

bool SDocumentTransaction::removeEntity(SEntityId entity_id)
{
    const auto iterator = std::find_if(m_document.m_entities.begin(), m_document.m_entities.end(),
                                       [entity_id](const SEntityRecord& entity)
                                       {
                                           return entity.id == entity_id;
                                       });
    if (iterator == m_document.m_entities.end())
    {
        return false;
    }
    m_removed_entities.push_back(*iterator);
    return true;
}

bool SDocumentTransaction::setEntityLayer(SEntityId entity_id, QString layer_name)
{
    const SLayerRecord* target_layer = m_document.layer(layer_name.trimmed());
    if (!target_layer)
    {
        return false;
    }
    const auto staged_iterator = std::find_if(m_entities.begin(), m_entities.end(),
                                              [entity_id](const SEntityRecord& entity)
                                              {
                                                  return entity.id == entity_id;
                                              });
    if (staged_iterator != m_entities.end())
    {
        staged_iterator->layer_name = target_layer->name;
        return true;
    }
    const auto source_iterator =
        std::find_if(m_document.m_entities.begin(), m_document.m_entities.end(),
                     [entity_id](const SEntityRecord& entity)
                     {
                         return entity.id == entity_id;
                     });
    if (source_iterator == m_document.m_entities.end() ||
        m_document.isLayerLocked(source_iterator->layer_name))
    {
        return false;
    }
    if (source_iterator->layer_name == target_layer->name)
    {
        return false;
    }
    SEntityRecord replacement = *source_iterator;
    replacement.layer_name = target_layer->name;
    return replaceEntity(entity_id, std::move(replacement));
}

bool SDocumentTransaction::setEntityLineWidth(SEntityId entity_id, double line_width_mm)
{
    if ((line_width_mm < 0.0 && line_width_mm != -1.0) || line_width_mm > 2.11)
    {
        return false;
    }
    const auto staged_iterator = std::find_if(m_entities.begin(), m_entities.end(),
                                              [entity_id](const SEntityRecord& entity)
                                              {
                                                  return entity.id == entity_id;
                                              });
    if (staged_iterator != m_entities.end())
    {
        if (staged_iterator->line_width_mm == line_width_mm)
        {
            return false;
        }
        staged_iterator->line_width_mm = line_width_mm;
        return true;
    }
    const auto source_iterator =
        std::find_if(m_document.m_entities.begin(), m_document.m_entities.end(),
                     [entity_id](const SEntityRecord& entity)
                     {
                         return entity.id == entity_id;
                     });
    if (source_iterator == m_document.m_entities.end() ||
        m_document.isLayerLocked(source_iterator->layer_name) ||
        source_iterator->line_width_mm == line_width_mm)
    {
        return false;
    }
    SEntityRecord replacement = *source_iterator;
    replacement.line_width_mm = line_width_mm;
    return replaceEntity(entity_id, std::move(replacement));
}

bool SDocumentTransaction::setDrawingSettings(const SDrawingSettings& settings)
{
    if (!isValidDrawingSettings(settings) || settings == m_document.drawingSettings())
    {
        return false;
    }
    m_drawing_settings = settings;
    return true;
}

bool SDocumentTransaction::setEntityOrder(std::vector<SEntityId> entity_order)
{
    if (entity_order.size() != m_document.m_entities.size())
    {
        return false;
    }
    std::vector<SEntityId> current_order;
    current_order.reserve(m_document.m_entities.size());
    for (const SEntityRecord& entity : m_document.m_entities)
    {
        current_order.push_back(entity.id);
    }
    if (entity_order == current_order)
    {
        return false;
    }
    std::vector<SEntityId> existing_ids = current_order;
    std::sort(existing_ids.begin(), existing_ids.end());
    std::vector<SEntityId> requested_ids = entity_order;
    std::sort(requested_ids.begin(), requested_ids.end());
    if (requested_ids != existing_ids)
    {
        return false;
    }
    m_entity_order = std::move(entity_order);
    return true;
}

void SDocumentTransaction::commit()
{
    if (m_is_committed ||
        (m_entities.empty() && m_removed_entities.empty() && !m_drawing_settings &&
         !m_entity_order && m_added_layers.empty()))
    {
        return;
    }

    const bool adds_dimension =
        std::any_of(m_entities.begin(), m_entities.end(), [](const SEntityRecord& entity)
                    {
                        return entity.type == SEntityType::LinearDimension;
                    });
    if (adds_dimension)
    {
        m_document.ensureDefpointsLayer();
    }

    if (!m_added_layers.empty())
    {
        const std::vector<SLayerRecord> layers_before = m_document.m_layers;
        const QString current_layer_before = m_document.m_current_layer_name;
        m_document.m_layers.insert(m_document.m_layers.end(),
                                   std::make_move_iterator(m_added_layers.begin()),
                                   std::make_move_iterator(m_added_layers.end()));
        m_document.commitLayerChange(std::move(m_label), layers_before,
                                     current_layer_before, std::move(m_entities),
                                     std::move(m_removed_entities));
    }
    else
    {
        m_document.commitEntities(std::move(m_label), std::move(m_entities),
                                  std::move(m_removed_entities), m_drawing_settings,
                                  std::move(m_entity_order));
    }
    m_is_committed = true;
}

} // namespace smartCam
