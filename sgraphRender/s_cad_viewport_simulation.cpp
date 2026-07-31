#include "s_cad_viewport.h"

#include <QPainter>
#include <algorithm>

namespace smartCam
{

SPoint2d SCadViewport::visibleWorldBottomLeft() const
{
    return screenToWorld(QPointF(0.0, height()));
}

void SCadViewport::setSimulationOverlay(std::vector<SToolpathMotion> motions,
                                        std::size_t completed_motion_count,
                                        bool trace_visible)
{
    m_simulation_motions = std::move(motions);
    m_completed_simulation_motion_count =
        std::min(completed_motion_count, m_simulation_motions.size());
    m_is_simulation_trace_visible = trace_visible;
    m_is_simulation_overlay_active = !m_simulation_motions.empty();
    update();
}

void SCadViewport::clearSimulationOverlay()
{
    m_simulation_motions.clear();
    m_completed_simulation_motion_count = 0;
    m_is_simulation_overlay_active = false;
    update();
}

void SCadViewport::drawSimulationOverlay(QPainter& painter)
{
    if (!m_is_simulation_overlay_active)
    {
        return;
    }
    painter.save();
    for (std::size_t index = 0; index < m_simulation_motions.size(); ++index)
    {
        const SToolpathMotion& motion = m_simulation_motions[index];
        const bool is_completed = index < m_completed_simulation_motion_count;
        if (is_completed && !m_is_simulation_trace_visible)
        {
            continue;
        }
        QColor color;
        Qt::PenStyle style = Qt::SolidLine;
        if (motion.type == SToolpathMotionType::Rapid)
        {
            color = is_completed ? QColor(156, 118, 67) : QColor(255, 174, 71);
            style = Qt::DashLine;
        }
        else
        {
            color = is_completed ? QColor(74, 211, 127) : QColor(72, 169, 232);
        }
        painter.setPen(QPen(color, is_completed ? 2.2 : 1.5, style,
                            Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(worldToScreen(motion.start_point), worldToScreen(motion.end_point));
    }
    if (m_completed_simulation_motion_count > 0)
    {
        const SPoint2d tool_position =
            m_simulation_motions[m_completed_simulation_motion_count - 1].end_point;
        painter.setPen(QPen(QColor(255, 245, 133), 2.0));
        painter.setBrush(QColor(255, 194, 92, 180));
        painter.drawEllipse(worldToScreen(tool_position), 6.0, 6.0);
    }
    painter.restore();
}

} // namespace smartCam
