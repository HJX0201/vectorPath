#include "vp_cad_viewport.h"
#include "vp_cad_viewport_geometry.h"

#include <QPainter>

namespace Vp
{

void VpCadViewport::drawBreakPreview(QPainter& painter)
{
    if (!m_reference_entity_id)
    {
        const std::optional<VpEntityId> hovered_id = entityAt(m_cursor_world);
        const VpEntityRecord* hovered_entity = hovered_id ? entityById(*hovered_id) : nullptr;
        if (hovered_entity && (hovered_entity->type == VpEntityType::Line ||
                               hovered_entity->type == VpEntityType::Circle ||
                               hovered_entity->type == VpEntityType::Arc))
        {
            painter.setPen(QPen(QColor(105, 219, 142), 2.0, Qt::DashLine));
            drawEntityGeometry(painter, *hovered_entity, QColor(105, 219, 142, 72));
        }
        return;
    }
    const VpEntityRecord* source = entityById(*m_reference_entity_id);
    if (!source || !m_first_point)
    {
        return;
    }
    const std::vector<VpEntityRecord> pieces =
        brokenEntityParts(*source, *m_first_point, m_cursor_world);
    if (pieces.empty())
    {
        return;
    }
    painter.setPen(QPen(m_canvas_color, 5.0, Qt::SolidLine, Qt::RoundCap));
    drawEntityGeometry(painter, *source, m_canvas_color);
    painter.setPen(QPen(QColor(92, 214, 255), 2.0, Qt::DashLine));
    for (const VpEntityRecord& piece : pieces)
    {
        drawEntityGeometry(painter, piece, QColor(92, 214, 255, 72));
    }
    painter.setBrush(QColor(255, 194, 92));
    painter.setPen(QPen(QColor(255, 222, 145), 1.0));
    painter.drawEllipse(worldToScreen(*m_first_point), 3.5, 3.5);
    painter.drawEllipse(worldToScreen(m_cursor_world), 3.5, 3.5);
}

} // namespace Vp
