#include "s_cad_viewport.h"

#include <QPainter>
#include <cmath>

namespace smartCam
{

void SCadViewport::drawGrid(QPainter& painter)
{
    const double spacing = adaptiveGridSpacing();
    const SGridBasis basis = draftingGridBasis(m_grid_rotation);
    const SPoint2d center = screenToWorld(QPointF(width() * 0.5, height() * 0.5));
    const double determinant =
        (basis.first_axis.x * basis.second_axis.y) - (basis.first_axis.y * basis.second_axis.x);
    const double center_first =
        ((center.x * basis.second_axis.y) - (center.y * basis.second_axis.x)) / determinant;
    const double center_second =
        ((basis.first_axis.x * center.y) - (basis.first_axis.y * center.x)) / determinant;
    const qint64 first_index = static_cast<qint64>(std::round(center_first / spacing));
    const qint64 second_index = static_cast<qint64>(std::round(center_second / spacing));
    const double guide_length = std::hypot(width(), height()) * 1.75 / m_zoom;
    const int line_count = std::min(1000, static_cast<int>(std::ceil(guide_length / spacing)) + 3);

    painter.setPen(QPen(m_grid_color, 1.0));
    for (int offset = -line_count; offset <= line_count; ++offset)
    {
        const double first_distance = static_cast<double>(first_index + offset) * spacing;
        const SPoint2d first_origin{basis.first_axis.x * first_distance,
                                    basis.first_axis.y * first_distance};
        const SPoint2d first_start{first_origin.x - (basis.second_axis.x * guide_length),
                                   first_origin.y - (basis.second_axis.y * guide_length)};
        const SPoint2d first_end{first_origin.x + (basis.second_axis.x * guide_length),
                                 first_origin.y + (basis.second_axis.y * guide_length)};
        painter.drawLine(worldToScreen(first_start), worldToScreen(first_end));

        const double second_distance = static_cast<double>(second_index + offset) * spacing;
        const SPoint2d second_origin{basis.second_axis.x * second_distance,
                                     basis.second_axis.y * second_distance};
        const SPoint2d second_start{second_origin.x - (basis.first_axis.x * guide_length),
                                    second_origin.y - (basis.first_axis.y * guide_length)};
        const SPoint2d second_end{second_origin.x + (basis.first_axis.x * guide_length),
                                  second_origin.y + (basis.first_axis.y * guide_length)};
        painter.drawLine(worldToScreen(second_start), worldToScreen(second_end));
    }

    painter.setPen(QPen(m_major_grid_color, 1.0));
    const QPointF origin = worldToScreen({0.0, 0.0});
    painter.drawLine(origin - QPointF(basis.first_axis.x, -basis.first_axis.y) * guide_length,
                     origin + QPointF(basis.first_axis.x, -basis.first_axis.y) * guide_length);
    painter.drawLine(origin - QPointF(basis.second_axis.x, -basis.second_axis.y) * guide_length,
                     origin + QPointF(basis.second_axis.x, -basis.second_axis.y) * guide_length);
}

void SCadViewport::drawNavigationOverlay(QPainter& painter)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QPointF axis_origin(34.0, height() - 34.0);
    painter.setPen(QPen(QColor(218, 82, 72), 1.6));
    painter.drawLine(axis_origin, axis_origin + QPointF(22.0, 0.0));
    painter.setPen(QPen(QColor(83, 191, 113), 1.6));
    painter.drawLine(axis_origin, axis_origin - QPointF(0.0, 22.0));
    painter.setPen(m_overlay_text_color);
    painter.drawText(axis_origin + QPointF(25.0, 4.0), QStringLiteral("X"));
    painter.drawText(axis_origin - QPointF(4.0, 26.0), QStringLiteral("Y"));

    const QRect view_label_rect(width() - 92, 12, 76, 24);
    painter.setPen(QPen(m_major_grid_color, 1.0));
    painter.setBrush(
        QColor(m_canvas_color.red(), m_canvas_color.green(), m_canvas_color.blue(), 210));
    painter.drawRect(view_label_rect);
    painter.setPen(m_overlay_text_color);
    painter.drawText(view_label_rect, Qt::AlignCenter, tr("俯视 2D"));
    painter.restore();
}

} // namespace smartCam
