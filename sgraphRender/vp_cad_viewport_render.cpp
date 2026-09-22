#include "vp_cad_document.h"
#include "vp_cad_viewport.h"
#include "vp_cad_viewport_geometry.h"
#include "vp_document_transaction.h"
#include "vp_ellipse_geometry.h"
#include "vp_entity.h"
#include "vp_standard_shape.h"

#include <QPainter>
#include <QPolygonF>
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace Vp
{
namespace
{

bool toolHasCursorPreview(VpToolMode tool_mode)
{
    return tool_mode == VpToolMode::Move || tool_mode == VpToolMode::Copy ||
           tool_mode == VpToolMode::Rotate || tool_mode == VpToolMode::Scale ||
           tool_mode == VpToolMode::Mirror || tool_mode == VpToolMode::Trim ||
           tool_mode == VpToolMode::Extend || tool_mode == VpToolMode::Break ||
           tool_mode == VpToolMode::Join || tool_mode == VpToolMode::Explode ||
           tool_mode == VpToolMode::Stretch || tool_mode == VpToolMode::Lengthen ||
           tool_mode == VpToolMode::Fillet || tool_mode == VpToolMode::Chamfer ||
           tool_mode == VpToolMode::Blend || tool_mode == VpToolMode::Align ||
           tool_mode == VpToolMode::ArrayRect || tool_mode == VpToolMode::ArrayPolar ||
           tool_mode == VpToolMode::ArrayPath || tool_mode == VpToolMode::ArrayEdit ||
           tool_mode == VpToolMode::SplineEdit || tool_mode == VpToolMode::Offset ||
           tool_mode == VpToolMode::Text || tool_mode == VpToolMode::MText ||
           tool_mode == VpToolMode::Leader || dimensionTypeForToolMode(tool_mode).has_value() ||
           tool_mode == VpToolMode::Hatch;
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

QPointF VpCadViewport::worldToScreen(const VpPoint2d& world_point) const
{
    return {(width() * 0.5) + m_pan_offset.x() + (world_point.x * m_zoom),
            (height() * 0.5) + m_pan_offset.y() - (world_point.y * m_zoom)};
}

VpPoint2d VpCadViewport::screenToWorld(const QPointF& screen_point) const
{
    return {(screen_point.x() - (width() * 0.5) - m_pan_offset.x()) / m_zoom,
            -((screen_point.y() - (height() * 0.5) - m_pan_offset.y()) / m_zoom)};
}

void VpCadViewport::drawEntities(QPainter& painter)
{
    if (!m_document)
    {
        return;
    }
    drawEntitiesForSpace(painter, true);
}

void VpCadViewport::drawEntitiesForSpace(QPainter& painter, bool show_selection,
                                         bool plottable_only)
{
    for (const VpEntityRecord& entity : m_document->entities())
    {
        const VpLayerRecord* layer_record = m_document->layer(entity.layer_name);
        if (!layer_record || !layer_record->is_visible ||
            (plottable_only && !layer_record->is_plottable))
        {
            continue;
        }
        const bool is_selected =
            show_selection && std::find(m_selected_entity_ids.begin(), m_selected_entity_ids.end(),
                                        entity.id) != m_selected_entity_ids.end();
        const bool is_svg_color_block =
            entity.type == VpEntityType::Hatch &&
            entity.layer_name.startsWith(QStringLiteral("SVG_"), Qt::CaseInsensitive) &&
            !entity.layer_name.startsWith(QStringLiteral("SVG_FILL_"), Qt::CaseInsensitive);
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
        if (entity.type == VpEntityType::Hatch && !is_selected)
        {
            draw_color.setAlpha(std::min(draw_color.alpha(), is_svg_color_block ? 190 : 96));
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
        if (entity.type == VpEntityType::Hatch)
        {
            if (is_selected && is_svg_color_block)
            {
                QColor boundary_color = m_document->layerColor(entity.layer_name);
                boundary_color.setAlpha(220);
                painter.setPen(QPen(boundary_color, 1.5));
            }
            else
            {
                painter.setPen(is_selected ? QPen(QColor(255, 194, 92), 1.5) : Qt::NoPen);
            }
        }
        drawEntityGeometry(painter, entity, draw_color);
    }
}

void VpCadViewport::drawPreview(QPainter& painter)
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
    if (m_tool_mode == VpToolMode::Line)
    {
        painter.drawLine(worldToScreen(*m_first_point), worldToScreen(m_cursor_world));
    }
    else if (m_tool_mode == VpToolMode::Circle && !m_input_points.empty())
    {
        drawCircleConstructionPreview(painter);
    }
    else if (m_tool_mode == VpToolMode::Ellipse && !m_input_points.empty())
    {
        if (m_input_points.size() == 1)
        {
            painter.drawLine(worldToScreen(m_input_points.front()), worldToScreen(m_cursor_world));
        }
        else
        {
            const VpPoint2d center{(m_input_points[0].x + m_input_points[1].x) * 0.5,
                                   (m_input_points[0].y + m_input_points[1].y) * 0.5};
            const VpPoint2d major_axis{m_input_points[1].x - center.x,
                                       m_input_points[1].y - center.y};
            const double major_length = std::hypot(major_axis.x, major_axis.y);
            const double minor_length = std::min(distance(center, m_cursor_world), major_length);
            if (major_length > 1.0e-9 && minor_length > 1.0e-9)
            {
                const VpEllipseEntity ellipse{center,
                                              major_axis,
                                              {-major_axis.y * minor_length / major_length,
                                               major_axis.x * minor_length / major_length}};
                const std::vector<VpPoint2d> points = ellipseApproximation(ellipse);
                QPolygonF polygon;
                for (const VpPoint2d& point : points)
                {
                    polygon.append(worldToScreen(point));
                }
                painter.drawPolyline(polygon);
                painter.drawLine(worldToScreen(m_input_points[0]),
                                 worldToScreen(m_input_points[1]));
            }
        }
    }
    else if (m_tool_mode == VpToolMode::Spline && !m_input_points.empty())
    {
        std::vector<VpPoint2d> controls = m_input_points;
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
    else if (m_tool_mode == VpToolMode::Polyline && !m_input_points.empty())
    {
        painter.setPen(QPen(QColor(255, 194, 92), 1.4, Qt::SolidLine));
        VpEntityRecord confirmed_preview;
        confirmed_preview.type = VpEntityType::Polyline;
        std::vector<double> preview_bulges = m_polyline_bulges;
        preview_bulges.resize(m_input_points.size(), 0.0);
        std::vector<double> preview_start_widths = m_polyline_start_widths;
        std::vector<double> preview_end_widths = m_polyline_end_widths;
        preview_start_widths.resize(m_input_points.size(), 0.0);
        preview_end_widths.resize(m_input_points.size(), 0.0);
        confirmed_preview.geometry = VpPolylineEntity{m_input_points, false, preview_bulges,
                                                      preview_start_widths, preview_end_widths};
        drawEntityGeometry(painter, confirmed_preview, QColor(255, 194, 92, 72));
        painter.setPen(preview_pen);
        if (m_polyline_arc_mode && m_polyline_arc_point)
        {
            VpPoint2d center;
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
                    VpEntityRecord arc_preview;
                    arc_preview.type = VpEntityType::Polyline;
                    arc_preview.geometry = VpPolylineEntity{{m_input_points.back(), m_cursor_world},
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
            VpEntityRecord line_preview;
            line_preview.type = VpEntityType::Polyline;
            line_preview.geometry = VpPolylineEntity{{m_input_points.back(), m_cursor_world},
                                                     false,
                                                     {0.0, 0.0},
                                                     {m_polyline_start_width, 0.0},
                                                     {m_polyline_end_width, 0.0}};
            drawEntityGeometry(painter, line_preview, QColor(255, 194, 92, 72));
        }
        painter.setPen(QPen(QColor(255, 222, 145), 1.0));
        painter.setBrush(QColor(255, 194, 92));
        for (const VpPoint2d& point : m_input_points)
        {
            painter.drawEllipse(worldToScreen(point), 3.0, 3.0);
        }
        if (m_polyline_arc_point)
        {
            painter.drawEllipse(worldToScreen(*m_polyline_arc_point), 3.0, 3.0);
        }
    }
    else if (m_tool_mode == VpToolMode::Rectangle && m_first_point)
    {
        const VpPoint2d opposite = m_cursor_world;
        const VpPoint2d top_right{opposite.x, m_first_point->y};
        const VpPoint2d bottom_left{m_first_point->x, opposite.y};
        painter.drawLine(worldToScreen(*m_first_point), worldToScreen(top_right));
        painter.drawLine(worldToScreen(top_right), worldToScreen(opposite));
        painter.drawLine(worldToScreen(opposite), worldToScreen(bottom_left));
        painter.drawLine(worldToScreen(bottom_left), worldToScreen(*m_first_point));
    }
    else if (m_tool_mode == VpToolMode::StandardShape && m_first_point)
    {
        VpEntityRecord shape_preview;
        shape_preview.type = VpEntityType::Polyline;
        shape_preview.geometry = VpPolylineEntity{
            standardShapeVertices(m_standard_shape_type, *m_first_point, m_cursor_world), true};
        drawEntityGeometry(painter, shape_preview, QColor(255, 194, 92, 72));
    }
    else if (m_tool_mode == VpToolMode::Arc && !m_input_points.empty())
    {
        drawArcConstructionPreview(painter);
        painter.setPen(QPen(QColor(255, 222, 145), 1.0));
        painter.setBrush(QColor(255, 194, 92));
        for (const VpPoint2d& point : m_input_points)
        {
            painter.drawEllipse(worldToScreen(point), 3.0, 3.0);
        }
    }
    else if ((m_tool_mode == VpToolMode::Move || m_tool_mode == VpToolMode::Copy) && m_first_point)
    {
        const double delta_x = m_cursor_world.x - m_first_point->x;
        const double delta_y = m_cursor_world.y - m_first_point->y;
        painter.drawLine(worldToScreen(*m_first_point), worldToScreen(m_cursor_world));
        painter.setPen(QPen(QColor(92, 214, 255), 1.6, Qt::DashLine));
        for (VpEntityId entity_id : m_selected_entity_ids)
        {
            if (const VpEntityRecord* source = entityById(entity_id))
            {
                drawEntityGeometry(painter, translatedEntity(*source, delta_x, delta_y),
                                   QColor(92, 214, 255, 72));
            }
        }
    }
    else if (m_tool_mode == VpToolMode::Rotate && m_first_point)
    {
        const double angle = entityAngleDegrees(*m_first_point, m_cursor_world);
        painter.drawLine(worldToScreen(*m_first_point), worldToScreen(m_cursor_world));
        painter.setPen(QPen(QColor(92, 214, 255), 1.6, Qt::DashLine));
        for (VpEntityId entity_id : m_selected_entity_ids)
        {
            if (const VpEntityRecord* source = entityById(entity_id))
            {
                drawEntityGeometry(painter, rotatedEntity(*source, *m_first_point, angle),
                                   QColor(92, 214, 255, 72));
            }
        }
    }
    else if (m_tool_mode == VpToolMode::Scale && m_first_point)
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
                for (VpEntityId entity_id : m_selected_entity_ids)
                {
                    if (const VpEntityRecord* source = entityById(entity_id))
                    {
                        drawEntityGeometry(painter,
                                           scaledEntity(*source, *m_first_point, scale_factor),
                                           QColor(92, 214, 255, 72));
                    }
                }
            }
        }
    }
    else if (m_tool_mode == VpToolMode::Mirror && m_first_point)
    {
        painter.drawLine(worldToScreen(*m_first_point), worldToScreen(m_cursor_world));
        if (distance(*m_first_point, m_cursor_world) > 1.0e-9)
        {
            painter.setPen(QPen(QColor(92, 214, 255), 1.6, Qt::DashLine));
            for (VpEntityId entity_id : m_selected_entity_ids)
            {
                if (const VpEntityRecord* source = entityById(entity_id))
                {
                    drawEntityGeometry(painter,
                                       mirroredEntity(*source, *m_first_point, m_cursor_world),
                                       QColor(92, 214, 255, 72));
                }
            }
        }
    }
    else if (m_tool_mode == VpToolMode::Offset && m_selected_entity_id)
    {
        const VpEntityRecord* source = entityById(*m_selected_entity_id);
        VpEntityRecord offset_entity;
        if (source && offsetEntity(*source, m_cursor_world, offset_entity))
        {
            painter.setPen(QPen(QColor(92, 214, 255), 1.7, Qt::DashLine));
            drawEntityGeometry(painter, offset_entity, QColor(92, 214, 255, 72));
        }
    }
    else if (m_tool_mode == VpToolMode::Trim)
    {
        drawTrimPreview(painter);
    }
    else if (m_tool_mode == VpToolMode::Extend)
    {
        drawExtendPreview(painter);
    }
    else if (m_tool_mode == VpToolMode::Break)
    {
        drawBreakPreview(painter);
    }
    else if (m_tool_mode == VpToolMode::Join)
    {
        drawJoinPreview(painter);
    }
    else if (m_tool_mode == VpToolMode::Explode)
    {
        drawExplodePreview(painter);
    }
    else if (m_tool_mode == VpToolMode::Stretch)
    {
        drawStretchPreview(painter);
    }
    else if (m_tool_mode == VpToolMode::Lengthen)
    {
        drawLengthenPreview(painter);
    }
    else if (m_tool_mode == VpToolMode::Fillet)
    {
        drawFilletPreview(painter);
    }
    else if (m_tool_mode == VpToolMode::Chamfer)
    {
        drawChamferPreview(painter);
    }
    else if (m_tool_mode == VpToolMode::Blend)
    {
        drawBlendPreview(painter);
    }
    else if (m_tool_mode == VpToolMode::Align)
    {
        drawAlignPreview(painter);
    }
    else if (m_tool_mode == VpToolMode::ArrayRect)
    {
        drawRectangularArrayPreview(painter);
    }
    else if (m_tool_mode == VpToolMode::ArrayPolar)
    {
        drawPolarArrayPreview(painter);
    }
    else if (m_tool_mode == VpToolMode::ArrayPath)
    {
        drawPathArrayPreview(painter);
    }
    else if (m_tool_mode == VpToolMode::ArrayEdit)
    {
        drawArrayEditPreview(painter);
    }
    else if (m_tool_mode == VpToolMode::SplineEdit)
    {
        drawSplineEditPreview(painter);
    }
    else if (m_tool_mode == VpToolMode::Text && !m_pending_text.trimmed().isEmpty())
    {
        VpEntityRecord text_preview;
        text_preview.type = VpEntityType::Text;
        text_preview.geometry = VpTextEntity{m_cursor_world,
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
    else if (m_tool_mode == VpToolMode::MText || m_tool_mode == VpToolMode::Leader)
    {
        drawAnnotationPreview(painter);
    }
    else if (dimensionTypeForToolMode(m_tool_mode) && !m_input_points.empty())
    {
        drawDimensionPreview(painter);
    }
    else if (m_tool_mode == VpToolMode::Hatch)
    {
        const std::optional<VpEntityId> boundary_id = entityAt(m_cursor_world);
        const VpEntityRecord* boundary = boundary_id ? entityById(*boundary_id) : nullptr;
        if (boundary && boundary->type == VpEntityType::Polyline)
        {
            const std::optional<VpHatchEntity> hatch = hatchFromBoundaryEntity(*boundary);
            if (hatch)
            {
                VpEntityRecord hatch_preview;
                hatch_preview.type = VpEntityType::Hatch;
                hatch_preview.geometry = *hatch;
                painter.setPen(QPen(QColor(105, 219, 142), 1.2, Qt::DashLine));
                drawEntityGeometry(painter, hatch_preview, QColor(105, 219, 142, 72));
            }
        }
    }
}

} // namespace Vp
