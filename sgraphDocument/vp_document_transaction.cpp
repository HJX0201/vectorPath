#include "vp_document_transaction.h"

#include "vp_cad_document.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Vp
{

VpDocumentTransaction::VpDocumentTransaction(VpCadDocument& document, QString label)
    : m_document(document), m_label(std::move(label))
{
}

VpDocumentTransaction::~VpDocumentTransaction() = default;

QString VpDocumentTransaction::creationLayerName()
{
    return m_document.resolveDrawingLayerName();
}

VpEntityId VpDocumentTransaction::addLine(const VpPoint2d& start_point, const VpPoint2d& end_point)
{
    VpEntityRecord entity;
    entity.id = m_document.reserveEntityId();
    entity.type = VpEntityType::Line;
    entity.layer_name = creationLayerName();
    entity.geometry = VpLineEntity{start_point, end_point};
    m_entities.push_back(std::move(entity));
    return m_entities.back().id;
}

VpEntityId VpDocumentTransaction::addCircle(const VpPoint2d& center, double radius)
{
    VpEntityRecord entity;
    entity.id = m_document.reserveEntityId();
    entity.type = VpEntityType::Circle;
    entity.layer_name = creationLayerName();
    entity.geometry = VpCircleEntity{center, radius};
    m_entities.push_back(std::move(entity));
    return m_entities.back().id;
}

VpEntityId VpDocumentTransaction::addArc(const VpPoint2d& center, double radius, double start_angle,
                                         double end_angle)
{
    VpEntityRecord entity;
    entity.id = m_document.reserveEntityId();
    entity.type = VpEntityType::Arc;
    entity.layer_name = creationLayerName();
    entity.geometry = VpArcEntity{center, radius, start_angle, end_angle};
    m_entities.push_back(std::move(entity));
    return m_entities.back().id;
}

VpEntityId VpDocumentTransaction::addEllipse(const VpPoint2d& center, const VpPoint2d& major_axis,
                                             const VpPoint2d& minor_axis)
{
    VpEntityRecord entity;
    entity.id = m_document.reserveEntityId();
    entity.type = VpEntityType::Ellipse;
    entity.layer_name = creationLayerName();
    entity.geometry = VpEllipseEntity{center, major_axis, minor_axis};
    m_entities.push_back(std::move(entity));
    return m_entities.back().id;
}

VpEntityId VpDocumentTransaction::addSpline(const std::array<VpPoint2d, 4>& control_points)
{
    VpEntityRecord entity;
    entity.id = m_document.reserveEntityId();
    entity.type = VpEntityType::Spline;
    entity.layer_name = creationLayerName();
    entity.geometry = VpSplineEntity{control_points};
    m_entities.push_back(std::move(entity));
    return m_entities.back().id;
}

VpEntityId VpDocumentTransaction::addPolyline(std::vector<VpPoint2d> vertices, bool is_closed,
                                              std::vector<double> bulges,
                                              std::vector<double> start_widths,
                                              std::vector<double> end_widths)
{
    bulges.resize(vertices.size(), 0.0);
    start_widths.resize(vertices.size(), 0.0);
    end_widths.resize(vertices.size(), 0.0);
    VpEntityRecord entity;
    entity.id = m_document.reserveEntityId();
    entity.type = VpEntityType::Polyline;
    entity.layer_name = creationLayerName();
    entity.geometry = VpPolylineEntity{std::move(vertices), is_closed, std::move(bulges),
                                       std::move(start_widths), std::move(end_widths)};
    m_entities.push_back(std::move(entity));
    return m_entities.back().id;
}

VpEntityId VpDocumentTransaction::addText(const VpPoint2d& position, QString text, double height,
                                          double rotation, VpTextHorizontalAlignment alignment,
                                          VpTextVerticalAlignment vertical_alignment)
{
    VpEntityRecord entity;
    entity.id = m_document.reserveEntityId();
    entity.type = VpEntityType::Text;
    entity.layer_name = creationLayerName();
    entity.geometry = VpTextEntity{
        position,  std::move(text),   height, rotation, m_document.currentTextStyleName(),
        alignment, vertical_alignment};
    m_entities.push_back(std::move(entity));
    return m_entities.back().id;
}

VpEntityId VpDocumentTransaction::addMText(const VpPoint2d& position, QString rich_text,
                                           double width, double height, double rotation,
                                           VpTextHorizontalAlignment alignment)
{
    VpEntityRecord entity;
    entity.id = m_document.reserveEntityId();
    entity.type = VpEntityType::MText;
    entity.layer_name = creationLayerName();
    entity.geometry = VpMTextEntity{
        position, std::move(rich_text), width, height, rotation, m_document.currentTextStyleName(),
        alignment};
    m_entities.push_back(std::move(entity));
    return m_entities.back().id;
}

VpEntityId VpDocumentTransaction::addLeader(std::vector<VpPoint2d> vertices, QString text,
                                            double text_height, double arrow_size)
{
    VpEntityRecord entity;
    entity.id = m_document.reserveEntityId();
    entity.type = VpEntityType::Leader;
    entity.layer_name = creationLayerName();
    entity.geometry = VpLeaderEntity{std::move(vertices), std::move(text), text_height, arrow_size,
                                     m_document.currentTextStyleName()};
    m_entities.push_back(std::move(entity));
    return m_entities.back().id;
}

VpEntityId VpDocumentTransaction::addLinearDimension(const VpPoint2d& first_point,
                                                     const VpPoint2d& second_point,
                                                     const VpPoint2d& dimension_line_point)
{
    VpLinearDimensionEntity dimension;
    dimension.first_point = first_point;
    dimension.second_point = second_point;
    dimension.dimension_line_point = dimension_line_point;
    dimension.dimension_type = VpDimensionType::Linear;
    return addDimension(std::move(dimension));
}

VpEntityId VpDocumentTransaction::addDimension(VpLinearDimensionEntity dimension)
{
    if (dimension.style_name.trimmed().isEmpty())
    {
        dimension.style_name = m_document.currentDimensionStyleName();
    }
    VpEntityRecord entity;
    entity.id = m_document.reserveEntityId();
    entity.type = VpEntityType::LinearDimension;
    entity.layer_name = creationLayerName();
    entity.geometry = std::move(dimension);
    m_entities.push_back(std::move(entity));
    return m_entities.back().id;
}

VpEntityId VpDocumentTransaction::addHatch(std::vector<VpPoint2d> boundary)
{
    return addHatch(VpHatchEntity{std::move(boundary)});
}

VpEntityId VpDocumentTransaction::addHatch(VpHatchEntity hatch)
{
    VpEntityRecord entity;
    entity.id = m_document.reserveEntityId();
    entity.type = VpEntityType::Hatch;
    entity.layer_name = creationLayerName();
    entity.geometry = std::move(hatch);
    m_entities.push_back(std::move(entity));
    return m_entities.back().id;
}

QString VpDocumentTransaction::ensureLayer(QString preferred_name, const QColor& color,
                                           int transparency)
{
    preferred_name = preferred_name.trimmed();
    if (preferred_name.isEmpty() || !color.isValid() || transparency < 0 || transparency > 90)
    {
        return {};
    }
    const auto pending_layer = [&](const QString& layer_name) -> const VpLayerRecord*
    {
        const auto iterator =
            std::find_if(m_added_layers.begin(), m_added_layers.end(),
                         [&layer_name](const VpLayerRecord& layer)
                         {
                             return layer.name.compare(layer_name, Qt::CaseInsensitive) == 0;
                         });
        return iterator == m_added_layers.end() ? nullptr : &*iterator;
    };
    const VpLayerRecord* existing = m_document.layer(preferred_name);
    const VpLayerRecord* pending = pending_layer(preferred_name);
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
    VpLayerRecord layer;
    layer.id = m_document.reserveLayerId();
    layer.name = layer_name;
    layer.color = color;
    layer.transparency = transparency;
    m_added_layers.push_back(std::move(layer));
    return layer_name;
}

VpEntityId VpDocumentTransaction::addEntity(VpEntityType type, VpEntityGeometry geometry,
                                            QString layer_name)
{
    const bool has_pending_layer =
        std::any_of(m_added_layers.begin(), m_added_layers.end(),
                    [&layer_name](const VpLayerRecord& layer)
                    {
                        return layer.name.compare(layer_name, Qt::CaseInsensitive) == 0;
                    });
    if (!m_document.layer(layer_name) && !has_pending_layer)
    {
        layer_name = creationLayerName();
    }
    VpEntityRecord entity;
    entity.id = m_document.reserveEntityId();
    entity.type = type;
    entity.layer_name = std::move(layer_name);
    entity.geometry = std::move(geometry);
    m_entities.push_back(std::move(entity));
    return m_entities.back().id;
}

VpEntityId VpDocumentTransaction::addEntityCopy(VpEntityRecord entity, bool preserve_association)
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

bool VpDocumentTransaction::replaceEntity(VpEntityId entity_id, VpEntityRecord replacement)
{
    const auto iterator = std::find_if(m_document.m_entities.begin(), m_document.m_entities.end(),
                                       [entity_id](const VpEntityRecord& entity)
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

bool VpDocumentTransaction::removeEntity(VpEntityId entity_id)
{
    const auto iterator = std::find_if(m_document.m_entities.begin(), m_document.m_entities.end(),
                                       [entity_id](const VpEntityRecord& entity)
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

bool VpDocumentTransaction::setEntityLayer(VpEntityId entity_id, QString layer_name)
{
    const VpLayerRecord* target_layer = m_document.layer(layer_name.trimmed());
    if (!target_layer)
    {
        return false;
    }
    const auto staged_iterator = std::find_if(m_entities.begin(), m_entities.end(),
                                              [entity_id](const VpEntityRecord& entity)
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
                     [entity_id](const VpEntityRecord& entity)
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
    VpEntityRecord replacement = *source_iterator;
    replacement.layer_name = target_layer->name;
    return replaceEntity(entity_id, std::move(replacement));
}

bool VpDocumentTransaction::setEntityLineWidth(VpEntityId entity_id, double line_width_mm)
{
    if ((line_width_mm < 0.0 && line_width_mm != -1.0) || line_width_mm > 2.11)
    {
        return false;
    }
    const auto staged_iterator = std::find_if(m_entities.begin(), m_entities.end(),
                                              [entity_id](const VpEntityRecord& entity)
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
                     [entity_id](const VpEntityRecord& entity)
                     {
                         return entity.id == entity_id;
                     });
    if (source_iterator == m_document.m_entities.end() ||
        m_document.isLayerLocked(source_iterator->layer_name) ||
        source_iterator->line_width_mm == line_width_mm)
    {
        return false;
    }
    VpEntityRecord replacement = *source_iterator;
    replacement.line_width_mm = line_width_mm;
    return replaceEntity(entity_id, std::move(replacement));
}

bool VpDocumentTransaction::setDrawingSettings(const VpDrawingSettings& settings)
{
    if (!isValidDrawingSettings(settings) || settings == m_document.drawingSettings())
    {
        return false;
    }
    m_drawing_settings = settings;
    return true;
}

bool VpDocumentTransaction::setEntityOrder(std::vector<VpEntityId> entity_order)
{
    if (entity_order.size() != m_document.m_entities.size())
    {
        return false;
    }
    std::vector<VpEntityId> current_order;
    current_order.reserve(m_document.m_entities.size());
    for (const VpEntityRecord& entity : m_document.m_entities)
    {
        current_order.push_back(entity.id);
    }
    if (entity_order == current_order)
    {
        return false;
    }
    std::vector<VpEntityId> existing_ids = current_order;
    std::sort(existing_ids.begin(), existing_ids.end());
    std::vector<VpEntityId> requested_ids = entity_order;
    std::sort(requested_ids.begin(), requested_ids.end());
    if (requested_ids != existing_ids)
    {
        return false;
    }
    m_entity_order = std::move(entity_order);
    return true;
}

void VpDocumentTransaction::commit()
{
    if (m_is_committed || (m_entities.empty() && m_removed_entities.empty() &&
                           !m_drawing_settings && !m_entity_order && m_added_layers.empty()))
    {
        return;
    }

    const bool adds_dimension = std::any_of(m_entities.begin(), m_entities.end(),
                                            [](const VpEntityRecord& entity)
                                            {
                                                return entity.type == VpEntityType::LinearDimension;
                                            });
    if (adds_dimension)
    {
        m_document.ensureDefpointsLayer();
    }

    if (!m_added_layers.empty())
    {
        const std::vector<VpLayerRecord> layers_before = m_document.m_layers;
        const QString current_layer_before = m_document.m_current_layer_name;
        m_document.m_layers.insert(m_document.m_layers.end(),
                                   std::make_move_iterator(m_added_layers.begin()),
                                   std::make_move_iterator(m_added_layers.end()));
        m_document.commitLayerChange(std::move(m_label), layers_before, current_layer_before,
                                     std::move(m_entities), std::move(m_removed_entities));
    }
    else
    {
        m_document.commitEntities(std::move(m_label), std::move(m_entities),
                                  std::move(m_removed_entities), m_drawing_settings,
                                  std::move(m_entity_order));
    }
    m_is_committed = true;
}

} // namespace Vp
