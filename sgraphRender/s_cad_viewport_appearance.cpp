#include "s_cad_viewport.h"

namespace smartGraphics
{

void SCadViewport::setAppearance(const QColor& canvas_color, const QColor& grid_color,
                                 const QColor& major_grid_color, const QColor& text_color)
{
    m_canvas_color = canvas_color;
    m_grid_color = grid_color;
    m_major_grid_color = major_grid_color;
    m_overlay_text_color = text_color;
    update();
}

} // namespace smartGraphics
