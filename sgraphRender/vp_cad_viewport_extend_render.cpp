#include "vp_cad_viewport.h"
#include "vp_cad_viewport_geometry.h"

#include <QPainter>

namespace Vp
{

void VpCadViewport::drawExtendPreview(QPainter& painter)
{
    const std::optional<VpEntityId> hovered_id = entityAt(m_cursor_world);
    const VpEntityRecord* hovered_entity = hovered_id ? entityById(*hovered_id) : nullptr;
    VpEntityRecord preview_entity;
    bool has_preview = false;
    if (!m_reference_entity_id)
    {
        if (hovered_entity && (hovered_entity->type == VpEntityType::Line ||
                               hovered_entity->type == VpEntityType::Circle ||
                               hovered_entity->type == VpEntityType::Arc))
        {
            preview_entity = *hovered_entity;
            has_preview = true;
        }
    }
    else if (const VpEntityRecord* boundary_entity = entityById(*m_reference_entity_id))
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

} // namespace Vp
