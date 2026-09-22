#include "vp_cad_viewport.h"
#include "vp_cad_viewport_geometry.h"

#include <QPainter>
#include <algorithm>

namespace Vp
{

void VpCadViewport::drawJoinPreview(QPainter& painter)
{
    std::vector<VpEntityRecord> sources;
    sources.reserve(m_selected_entity_ids.size() + 1);
    for (VpEntityId entity_id : m_selected_entity_ids)
    {
        if (const VpEntityRecord* source = entityById(entity_id))
        {
            if (source->type != VpEntityType::Line && source->type != VpEntityType::Arc &&
                source->type != VpEntityType::Polyline)
            {
                return;
            }
            sources.push_back(*source);
        }
    }
    const std::optional<VpEntityId> hovered_id = entityAt(m_cursor_world);
    const VpEntityRecord* hovered_entity = hovered_id ? entityById(*hovered_id) : nullptr;
    const bool source_is_linear =
        !sources.empty() && (sources.front().type == VpEntityType::Line ||
                             sources.front().type == VpEntityType::Polyline);
    const bool hovered_is_linear =
        hovered_entity && (hovered_entity->type == VpEntityType::Line ||
                           hovered_entity->type == VpEntityType::Polyline);
    const bool matches_source_type =
        sources.empty() || (hovered_entity && (hovered_entity->type == sources.front().type ||
                                               (source_is_linear && hovered_is_linear)));
    if (hovered_entity &&
        (hovered_entity->type == VpEntityType::Line || hovered_entity->type == VpEntityType::Arc ||
         hovered_entity->type == VpEntityType::Polyline) &&
        matches_source_type &&
        std::find(m_selected_entity_ids.begin(), m_selected_entity_ids.end(), hovered_entity->id) ==
            m_selected_entity_ids.end())
    {
        sources.push_back(*hovered_entity);
    }
    if (sources.size() == 1)
    {
        painter.setPen(QPen(QColor(105, 219, 142), 2.0, Qt::DashLine));
        drawEntityGeometry(painter, sources.front(), QColor(105, 219, 142, 72));
        return;
    }
    VpEntityRecord joined_entity;
    if (sources.size() >= 2 && joinedEntity(sources, 1.0e-6, joined_entity))
    {
        painter.setPen(QPen(QColor(92, 214, 255), 2.0, Qt::DashLine));
        drawEntityGeometry(painter, joined_entity, QColor(92, 214, 255, 72));
    }
}

} // namespace Vp
