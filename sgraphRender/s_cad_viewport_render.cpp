#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_viewport_geometry.h"
#include "s_document_transaction.h"
#include "s_ellipse_geometry.h"
#include "s_entity.h"
#include "s_standard_shape.h"

#include <QPainter>
#include <QPolygonF>
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace smartGraphics
{
namespace
{

bool toolHasCursorPreview(SToolMode tool_mode)
{
    return tool_mode == SToolMode::Move || tool_mode == SToolMode::Copy ||
           tool_mode == SToolMode::Rotate || tool_mode == SToolMode::Scale ||
           tool_mode == SToolMode::Mirror || tool_mode == SToolMode::Trim ||
           tool_mode == SToolMode::Extend || tool_mode == SToolMode::Break ||
           tool_mode == SToolMode::Join || tool_mode == SToolMode::Explode ||
           tool_mode == SToolMode::Stretch || tool_mode == SToolMode::Lengthen ||
           tool_mode == SToolMode::Fillet || tool_mode == SToolMode::Chamfer ||
           tool_mode == SToolMode::Blend || tool_mode == SToolMode::Align ||
           tool_mode == SToolMode::ArrayRect || tool_mode == SToolMode::ArrayPolar ||
           tool_mode == SToolMode::ArrayPath || tool_mode == SToolMode::ArrayEdit ||
           tool_mode == SToolMode::SplineEdit || tool_mode == SToolMode::Offset ||
           tool_mode == SToolMode::Text || tool_mode == SToolMode::MText ||
           tool_mode == SToolMode::Leader || dimensionTypeForToolMode(tool_mode).has_value() ||
           tool_mode == SToolMode::Hatch;
}

void applyLayerLineType(QPen& pen, const QString& line_type)
{
    if (line_type == QLatin1String("Dashed"))
    {
        pen.setStyle(Qt::DashLine);
    }
    else if (line_type == QLatin1String("Dotted"))
    {
        pen.setStyle(Qt::DotLine);
    }
    else if (line_type == QLatin1String("DashDot"))
    {
        pen.setStyle(Qt::DashDotLine);
    }
    else if (line_type == QLatin1String("Center"))
    {
        pen.setDashPattern({8.0, 2.0, 2.0, 2.0});
    }
    else if (line_type == QLatin1String("Hidden"))
    {
        pen.setDashPattern({4.0, 2.0});
    }
}

} // namespace

QPointF SCadViewport::worldToScreen(const SPoint2d& world_point) const
{
    return {(width() * 0.5) + m_pan_offset.x() + (world_point.x * m_zoom),
            (height() * 0.5) + m_pan_offset.y() - (world_point.y * m_zoom)};
}

SPoint2d SCadViewport::screenToWorld(const QPointF& screen_point) const
{
    return {(screen_point.x() - (width() * 0.5) - m_pan_offset.x()) / m_zoom,
            -((screen_point.y() - (height() * 0.5) - m_pan_offset.y()) / m_zoom)};
}

void SCadViewport::drawEntities(QPainter& painter)
{
    if (!m_document)
    {
        return;
    }
    drawEntitiesForSpace(painter, true);
}

void SCadViewport::drawEntitiesForSpace(QPainter& painter,
                                        bool show_selection, bool plottable_only)
{
    for (const SEntityRecord& entity : m_document->entities())
    {
        const SLayerRecord* layer_record = m_document->layer(entity.layer_name);
        if (!layer_record ||
            !layer_record->is_visible || (plottable_only && !layer_record->is_plottable))
        {
            continue;
        }
        const bool is_selected =
            show_selection && std::find(m_selected_entity_ids.begin(), m_selected_entity_ids.end(),
                                        entity.id) != m_selected_entity_ids.end();
        const bool is_svg_color_block =
            entity.type == SEntityType::Hatch &&
            entity.layer_name.startsWith(QStringLiteral("SVG_"),
                                          Qt::CaseInsensitive) &&
            !entity.layer_name.startsWith(QStringLiteral("SVG_FILL_"),
                                           Qt::CaseInsensitive);
        QColor draw_color =
            is_selected ? QColor(255, 194, 92) : m_document->layerColor(entity.layer_name);
        if (is_selected && is_svg_color_block)
        {
            draw_color = m_document->layerColor(entity.layer_name);
            draw_color.setAlpha(48);
        }
        const QColor source_plot_color = draw_color;
        if (plottable_only && m_plot_style_table_override)
        {
            draw_color = m_plot_style_table_override->mappedColor(draw_color);
        }
        if (!is_selected)
        {
            const int transparency = m_document->layerTransparency(entity.layer_name);
            draw_color.setAlpha(std::clamp(255 - ((transparency * 255) / 100), 25, 255));
        }
        if (entity.type == SEntityType::Hatch && !is_selected)
        {
            draw_color.setAlpha(
                std::min(draw_color.alpha(), is_svg_color_block ? 190 : 96));
        }
        double line_width_mm = entity.line_width_mm < 0.0
                                   ? m_document->layerLineWidth(entity.layer_name)
                                   : entity.line_width_mm;
        if (plottable_only && m_plot_style_table_override)
        {
            line_width_mm =
                m_plot_style_table_override->mappedLineWidth(source_plot_color, line_width_mm);
        }
        const double pen_width =
            is_selected ? std::max(2.2, m_is_lineweight_visible ? line_width_mm * 4.0 : 0.0)
                        : (m_is_lineweight_visible ? std::max(1.0, line_width_mm * 4.0) : 1.2);
        QPen entity_pen(draw_color, pen_width);
        applyLayerLineType(entity_pen, m_document->layerLineType(entity.layer_name));
        painter.setPen(entity_pen);
        if (entity.type == SEntityType::Hatch)
        {
            if (is_selected && is_svg_color_block)
            {
                QColor boundary_color =
                    m_document->layerColor(entity.layer_name);
                boundary_color.setAlpha(220);
                painter.setPen(QPen(boundary_color, 1.5));
            }
            else
            {
                painter.setPen(
                    is_selected ? QPen(QColor(255, 194, 92), 1.5) : Qt::NoPen);
            }
        }
        drawEntityGeometry(painter, entity, draw_color);
    }
}

void SCadViewport::drawPreview(QPainter& painter)
{
    if (m_is_box_selecting)
    {
        const bool is_crossing = m_selection_current_screen.x() < m_selection_start_screen.x();
        const QColor selection_color = is_crossing ? QColor(62, 156, 224) : QColor(75, 183, 118);
        painter.save();
        painter.setPen(QPen(selection_color, 1.0, is_crossing ? Qt::DashLine : Qt::SolidLine));
        QColor fill_color = selection_color;
        fill_color.setAlpha(42);
        painter.setBrush(fill_color);
        painter.drawRect(QRect(m_selection_start_screen, m_selection_current_screen).normalized());
        painter.restore();
    }
    drawObjectSnapMarker(painter);
    if (!m_first_point.has_value() && m_input_points.empty() && !toolHasCursorPreview(m_tool_mode))
    {
        return;
    }
    QPen preview_pen(QColor(255, 194, 92), 1.2, Qt::DashLine);
    painter.setPen(preview_pen);
    if (m_tool_mode == SToolMode::Line)
    {
        painter.drawLine(worldToScreen(*m_first_point), worldToScreen(m_cursor_world));
    }
    else if (m_tool_mode == SToolMode::Circle && !m_input_points.empty())
    {
        drawCircleConstructionPreview(painter);
    }
    else if (m_tool_mode == SToolMode::Ellipse && !m_input_points.empty())
    {
        if (m_input_points.size() == 1)
        {
            painter.drawLine(worldToScreen(m_input_points.front()), worldToScreen(m_cursor_world));
        }
        else
        {
            const SPoint2d center{(m_input_points[0].x + m_input_points[1].x) * 0.5,
                                  (m_input_points[0].y + m_input_points[1].y) * 0.5};
            const SPoint2d major_axis{m_input_points[1].x - center.x,
                                      m_input_points[1].y - center.y};
            const double major_length = std::hypot(major_axis.x, major_axis.y);
            const double minor_length = std::min(distance(center, m_cursor_world), major_length);
            if (major_length > 1.0e-9 && minor_length > 1.0e-9)
            {
                const SEllipseEntity ellipse{center,
                                             major_axis,
                                             {-major_axis.y * minor_length / major_length,
                                              major_axis.x * minor_length / major_length}};
                const std::vector<SPoint2d> points = ellipseApproximation(ellipse);
                QPolygonF polygon;
                for (const SPoint2d& point : points)
                {
                    polygon.append(worldToScreen(point));
                }
                painter.drawPolyline(polygon);
                painter.drawLine(worldToScreen(m_input_points[0]),
                                 worldToScreen(m_input_points[1]));
            }
        }
    }
    else if (m_tool_mode == SToolMode::Spline && !m_input_points.empty())
    {
        std::vector<SPoint2d> controls = m_input_points;
        controls.push_back(m_cursor_world);
        painter.setPen(QPen(QColor(120, 170, 210), 1.0, Qt::DashLine));
        for (std::size_t index = 1; index < controls.size(); ++index)
        {
            painter.drawLine(worldToScreen(controls[index - 1]), worldToScreen(controls[index]));
        }
        if (m_input_points.size() == 3)
        {
            QPainterPath path(worldToScreen(m_input_points[0]));
            path.cubicTo(worldToScreen(m_input_points[1]), worldToScreen(m_input_points[2]),
                         worldToScreen(m_cursor_world));
            painter.setPen(QPen(QColor(255, 194, 92), 1.5, Qt::SolidLine));
            painter.drawPath(path);
        }
    }
    else if (m_tool_mode == SToolMode::Polyline && !m_input_points.empty())
    {
        painter.setPen(QPen(QColor(255, 194, 92), 1.4, Qt::SolidLine));
        SEntityRecord confirmed_preview;
        confirmed_preview.type = SEntityType::Polyline;
        std::vector<double> preview_bulges = m_polyline_bulges;
        preview_bulges.resize(m_input_points.size(), 0.0);
        std::vector<double> preview_start_widths = m_polyline_start_widths;
        std::vector<double> preview_end_widths = m_polyline_end_widths;
        preview_start_widths.resize(m_input_points.size(), 0.0);
        preview_end_widths.resize(m_input_points.size(), 0.0);
        confirmed_preview.geometry = SPolylineEntity{m_input_points, false, preview_bulges,
                                                     preview_start_widths, preview_end_widths};
        drawEntityGeometry(painter, confirmed_preview, QColor(255, 194, 92, 72));
        painter.setPen(preview_pen);
        if (m_polyline_arc_mode && m_polyline_arc_point)
        {
            SPoint2d center;
            double radius = 0.0;
            double start_angle = 0.0;
            double end_angle = 0.0;
            if (calculateThreePointArc(m_input_points.back(), *m_polyline_arc_point, m_cursor_world,
                                       center, radius, start_angle, end_angle))
            {
                double bulge = 0.0;
                if (threePointBulge(m_input_points.back(), *m_polyline_arc_point, m_cursor_world,
                                    bulge))
                {
                    SEntityRecord arc_preview;
                    arc_preview.type = SEntityType::Polyline;
                    arc_preview.geometry = SPolylineEntity{{m_input_points.back(), m_cursor_world},
                                                           false,
                                                           {bulge, 0.0},
                                                           {m_polyline_start_width, 0.0},
                                                           {m_polyline_end_width, 0.0}};
                    drawEntityGeometry(painter, arc_preview, QColor(255, 194, 92, 72));
                }
            }
            else
            {
                painter.drawLine(worldToScreen(m_input_points.back()),
                                 worldToScreen(*m_polyline_arc_point));
                painter.drawLine(worldToScreen(*m_polyline_arc_point),
                                 worldToScreen(m_cursor_world));
            }
        }
        else
        {
            SEntityRecord line_preview;
            line_preview.type = SEntityType::Polyline;
            line_preview.geometry = SPolylineEntity{{m_input_points.back(), m_cursor_world},
                                                    false,
                                                    {0.0, 0.0},
                                                    {m_polyline_start_width, 0.0},
                                                    {m_polyline_end_width, 0.0}};
            drawEntityGeometry(painter, line_preview, QColor(255, 194, 92, 72));
        }
        painter.setPen(QPen(QColor(255, 222, 145), 1.0));
        painter.setBrush(QColor(255, 194, 92));
        for (const SPoint2d& point : m_input_points)
        {
            painter.drawEllipse(worldToScreen(point), 3.0, 3.0);
        }
        if (m_polyline_arc_point)
        {
            painter.drawEllipse(worldToScreen(*m_polyline_arc_point), 3.0, 3.0);
        }
    }
    else if (m_tool_mode == SToolMode::Rectangle && m_first_point)
    {
        const SPoint2d opposite = m_cursor_world;
        const SPoint2d top_right{opposite.x, m_first_point->y};
        const SPoint2d bottom_left{m_first_point->x, opposite.y};
        painter.drawLine(worldToScreen(*m_first_point), worldToScreen(top_right));
        painter.drawLine(worldToScreen(top_right), worldToScreen(opposite));
        painter.drawLine(worldToScreen(opposite), worldToScreen(bottom_left));
        painter.drawLine(worldToScreen(bottom_left), worldToScreen(*m_first_point));
    }
    else if (m_tool_mode == SToolMode::StandardShape && m_first_point)
    {
        SEntityRecord shape_preview;
        shape_preview.type = SEntityType::Polyline;
        shape_preview.geometry =
            SPolylineEntity{standardShapeVertices(m_standard_shape_type, *m_first_point,
                                                  m_cursor_world),
                            true};
        drawEntityGeometry(painter, shape_preview, QColor(255, 194, 92, 72));
    }
    else if (m_tool_mode == SToolMode::Arc && !m_input_points.empty())
    {
        drawArcConstructionPreview(painter);
        painter.setPen(QPen(QColor(255, 222, 145), 1.0));
        painter.setBrush(QColor(255, 194, 92));
        for (const SPoint2d& point : m_input_points)
        {
            painter.drawEllipse(worldToScreen(point), 3.0, 3.0);
        }
    }
    else if ((m_tool_mode == SToolMode::Move || m_tool_mode == SToolMode::Copy) && m_first_point)
    {
        const double delta_x = m_cursor_world.x - m_first_point->x;
        const double delta_y = m_cursor_world.y - m_first_point->y;
        painter.drawLine(worldToScreen(*m_first_point), worldToScreen(m_cursor_world));
        painter.setPen(QPen(QColor(92, 214, 255), 1.6, Qt::DashLine));
        for (SEntityId entity_id : m_selected_entity_ids)
        {
            if (const SEntityRecord* source = entityById(entity_id))
            {
                drawEntityGeometry(painter, translatedEntity(*source, delta_x, delta_y),
                                   QColor(92, 214, 255, 72));
            }
        }
    }
    else if (m_tool_mode == SToolMode::Rotate && m_first_point)
    {
        const double angle = entityAngleDegrees(*m_first_point, m_cursor_world);
        painter.drawLine(worldToScreen(*m_first_point), worldToScreen(m_cursor_world));
        painter.setPen(QPen(QColor(92, 214, 255), 1.6, Qt::DashLine));
        for (SEntityId entity_id : m_selected_entity_ids)
        {
            if (const SEntityRecord* source = entityById(entity_id))
            {
                drawEntityGeometry(painter, rotatedEntity(*source, *m_first_point, angle),
                                   QColor(92, 214, 255, 72));
            }
        }
    }
    else if (m_tool_mode == SToolMode::Scale && m_first_point)
    {
        painter.drawLine(worldToScreen(*m_first_point), worldToScreen(m_cursor_world));
        if (!m_input_points.empty())
        {
            const double reference_length = distance(*m_first_point, m_input_points.front());
            const double target_length = distance(*m_first_point, m_cursor_world);
            if (reference_length > 1.0e-9 && target_length > 1.0e-9)
            {
                const double scale_factor = target_length / reference_length;
                painter.setPen(QPen(QColor(92, 214, 255), 1.6, Qt::DashLine));
                for (SEntityId entity_id : m_selected_entity_ids)
                {
                    if (const SEntityRecord* source = entityById(entity_id))
                    {
                        drawEntityGeometry(painter,
                                           scaledEntity(*source, *m_first_point, scale_factor),
                                           QColor(92, 214, 255, 72));
                    }
                }
            }
        }
    }
    else if (m_tool_mode == SToolMode::Mirror && m_first_point)
    {
        painter.drawLine(worldToScreen(*m_first_point), worldToScreen(m_cursor_world));
        if (distance(*m_first_point, m_cursor_world) > 1.0e-9)
        {
            painter.setPen(QPen(QColor(92, 214, 255), 1.6, Qt::DashLine));
            for (SEntityId entity_id : m_selected_entity_ids)
            {
                if (const SEntityRecord* source = entityById(entity_id))
                {
                    drawEntityGeometry(painter,
                                       mirroredEntity(*source, *m_first_point, m_cursor_world),
                                       QColor(92, 214, 255, 72));
                }
            }
        }
    }
    else if (m_tool_mode == SToolMode::Offset && m_selected_entity_id)
    {
        const SEntityRecord* source = entityById(*m_selected_entity_id);
        SEntityRecord offset_entity;
        if (source && offsetEntity(*source, m_cursor_world, offset_entity))
        {
            painter.setPen(QPen(QColor(92, 214, 255), 1.7, Qt::DashLine));
            drawEntityGeometry(painter, offset_entity, QColor(92, 214, 255, 72));
        }
    }
    else if (m_tool_mode == SToolMode::Trim)
    {
        drawTrimPreview(painter);
    }
    else if (m_tool_mode == SToolMode::Extend)
    {
        drawExtendPreview(painter);
    }
    else if (m_tool_mode == SToolMode::Break)
    {
        drawBreakPreview(painter);
    }
    else if (m_tool_mode == SToolMode::Join)
    {
        drawJoinPreview(painter);
    }
    else if (m_tool_mode == SToolMode::Explode)
    {
        drawExplodePreview(painter);
    }
    else if (m_tool_mode == SToolMode::Stretch)
    {
        drawStretchPreview(painter);
    }
    else if (m_tool_mode == SToolMode::Lengthen)
    {
        drawLengthenPreview(painter);
    }
    else if (m_tool_mode == SToolMode::Fillet)
    {
        drawFilletPreview(painter);
    }
    else if (m_tool_mode == SToolMode::Chamfer)
    {
        drawChamferPreview(painter);
    }
    else if (m_tool_mode == SToolMode::Blend)
    {
        drawBlendPreview(painter);
    }
    else if (m_tool_mode == SToolMode::Align)
    {
        drawAlignPreview(painter);
    }
    else if (m_tool_mode == SToolMode::ArrayRect)
    {
        drawRectangularArrayPreview(painter);
    }
    else if (m_tool_mode == SToolMode::ArrayPolar)
    {
        drawPolarArrayPreview(painter);
    }
    else if (m_tool_mode == SToolMode::ArrayPath)
    {
        drawPathArrayPreview(painter);
    }
    else if (m_tool_mode == SToolMode::ArrayEdit)
    {
        drawArrayEditPreview(painter);
    }
    else if (m_tool_mode == SToolMode::SplineEdit)
    {
        drawSplineEditPreview(painter);
    }
    else if (m_tool_mode == SToolMode::Text && !m_pending_text.trimmed().isEmpty())
    {
        SEntityRecord text_preview;
        text_preview.type = SEntityType::Text;
        text_preview.geometry = STextEntity{m_cursor_world,
                                            m_pending_text,
                                            2.5,
                                            0.0,
                                            m_document ? m_document->currentTextStyleName()
                                                       : QStringLiteral("Standard"),
                                            m_pending_text_alignment,
                                            m_pending_text_vertical_alignment};
        painter.setPen(QPen(QColor(92, 214, 255), 1.3, Qt::DashLine));
        drawEntityGeometry(painter, text_preview, QColor(92, 214, 255, 72));
    }
    else if (m_tool_mode == SToolMode::MText || m_tool_mode == SToolMode::Leader)
    {
        drawAnnotationPreview(painter);
    }
    else if (dimensionTypeForToolMode(m_tool_mode) && !m_input_points.empty())
    {
        drawDimensionPreview(painter);
    }
    else if (m_tool_mode == SToolMode::Hatch)
    {
        const std::optional<SEntityId> boundary_id = entityAt(m_cursor_world);
        const SEntityRecord* boundary = boundary_id ? entityById(*boundary_id) : nullptr;
        if (boundary && boundary->type == SEntityType::Polyline)
        {
            const std::optional<SHatchEntity> hatch = hatchFromBoundaryEntity(*boundary);
            if (hatch)
            {
                SEntityRecord hatch_preview;
                hatch_preview.type = SEntityType::Hatch;
                hatch_preview.geometry = *hatch;
                painter.setPen(QPen(QColor(105, 219, 142), 1.2, Qt::DashLine));
                drawEntityGeometry(painter, hatch_preview, QColor(105, 219, 142, 72));
            }
        }
    }
}

} // namespace smartGraphics
