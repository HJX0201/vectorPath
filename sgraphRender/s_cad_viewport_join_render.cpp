#include "s_cad_viewport.h"
#include "s_cad_viewport_geometry.h"

#include <QPainter>
#include <algorithm>

namespace smartGraphics
{

void SCadViewport::drawJoinPreview(QPainter& painter)
{
    std::vector<SEntityRecord> sources;
    sources.reserve(m_selected_entity_ids.size() + 1);
    for (SEntityId entity_id : m_selected_entity_ids)
    {
        if (const SEntityRecord* source = entityById(entity_id))
        {
            if (source->type != SEntityType::Line && source->type != SEntityType::Arc &&
                source->type != SEntityType::Polyline)
            {
                return;
            }
            sources.push_back(*source);
        }
    }
    const std::optional<SEntityId> hovered_id = entityAt(m_cursor_world);
    const SEntityRecord* hovered_entity = hovered_id ? entityById(*hovered_id) : nullptr;
    const bool source_is_linear =
        !sources.empty() && (sources.front().type == SEntityType::Line ||
                             sources.front().type == SEntityType::Polyline);
    const bool hovered_is_linear =
        hovered_entity && (hovered_entity->type == SEntityType::Line ||
                           hovered_entity->type == SEntityType::Polyline);
    const bool matches_source_type =
        sources.empty() || (hovered_entity && (hovered_entity->type == sources.front().type ||
                                               (source_is_linear && hovered_is_linear)));
    if (hovered_entity &&
        (hovered_entity->type == SEntityType::Line || hovered_entity->type == SEntityType::Arc ||
         hovered_entity->type == SEntityType::Polyline) &&
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
    SEntityRecord joined_entity;
    if (sources.size() >= 2 && joinedEntity(sources, 1.0e-6, joined_entity))
    {
        painter.setPen(QPen(QColor(92, 214, 255), 2.0, Qt::DashLine));
        drawEntityGeometry(painter, joined_entity, QColor(92, 214, 255, 72));
    }
}

} // namespace smartGraphics
