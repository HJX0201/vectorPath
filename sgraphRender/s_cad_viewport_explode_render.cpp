#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_viewport_geometry.h"

#include <QPainter>

namespace smartGraphics
{

void SCadViewport::drawExplodePreview(QPainter& painter)
{
    const std::optional<SEntityId> hovered_id = entityAt(m_cursor_world);
    const SEntityRecord* source = hovered_id ? entityById(*hovered_id) : nullptr;
    if (!source)
    {
        return;
    }
    const std::vector<SEntityRecord> parts = explodedEntityParts(*source);
    if (parts.empty())
    {
        return;
    }
    painter.setPen(QPen(m_canvas_color, 5.0, Qt::SolidLine, Qt::RoundCap));
    drawEntityGeometry(painter, *source, m_canvas_color);
    painter.setPen(QPen(QColor(92, 214, 255), 2.0, Qt::DashLine));
    for (const SEntityRecord& part : parts)
    {
        drawEntityGeometry(painter, part, QColor(92, 214, 255, 72));
    }
}

} // namespace smartGraphics
