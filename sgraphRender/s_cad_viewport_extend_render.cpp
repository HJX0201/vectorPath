#include "s_cad_viewport.h"
#include "s_cad_viewport_geometry.h"

#include <QPainter>

namespace vectorPath
{

void SCadViewport::drawExtendPreview(QPainter& painter)
{
    const std::optional<SEntityId> hovered_id = entityAt(m_cursor_world);
    const SEntityRecord* hovered_entity = hovered_id ? entityById(*hovered_id) : nullptr;
    SEntityRecord preview_entity;
    bool has_preview = false;
    if (!m_reference_entity_id)
    {
        if (hovered_entity && (hovered_entity->type == SEntityType::Line ||
                               hovered_entity->type == SEntityType::Circle ||
                               hovered_entity->type == SEntityType::Arc))
        {
            preview_entity = *hovered_entity;
            has_preview = true;
        }
    }
    else if (const SEntityRecord* boundary_entity = entityById(*m_reference_entity_id))
    {
        has_preview = hovered_entity && extendedEntity(*boundary_entity, *hovered_entity,
                                                       m_cursor_world, preview_entity);
    }
    if (has_preview)
    {
        painter.setPen(QPen(QColor(92, 214, 255), 2.0, Qt::DashLine));
        drawEntityGeometry(painter, preview_entity, QColor(92, 214, 255, 72));
    }
}

} // namespace vectorPath
