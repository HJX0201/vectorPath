#include "vp_cad_document.h"
#include "vp_cad_viewport.h"
#include "vp_cad_viewport_geometry.h"
#include "vp_ellipse_geometry.h"
#include "vp_spline_geometry.h"

#include <QFont>
#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QTextDocument>
#include <algorithm>
#include <cmath>

namespace Vp
{
namespace
{

QFont entityTextFont(const VpCadDocument* document, const QString& style_name, double height,
                     double zoom, const QFont& fallback)
{
    QFont font = fallback;
    if (document)
    {
        if (const VpTextStyleRecord* style = document->textStyle(style_name))
        {
            font.setFamily(style->font_family);
            font.setBold(style->is_bold);
            font.setItalic(style->is_italic);
            font.setStretch(static_cast<int>(std::clamp(style->width_factor, 0.5, 2.0) * 100.0));
            if (style->fixed_height > 0.0)
            {
                height = style->fixed_height;
            }
        }
    }
    font.setPixelSize(std::max(9, static_cast<int>(height * zoom)));
    return font;
}

} // namespace

void VpCadViewport::drawEntityGeometry(QPainter& painter, const VpEntityRecord& entity,
                                       const QColor& fill_color)
{
    if (entity.type == VpEntityType::Line)
    {
        const auto& line = std::get<VpLineEntity>(entity.geometry);
        painter.drawLine(worldToScreen(line.start_point), worldToScreen(line.end_point));
    }
    else if (entity.type == VpEntityType::Circle)
    {
        const auto& circle = std::get<VpCircleEntity>(entity.geometry);
        const QPointF center = worldToScreen(circle.center);
        const double radius = circle.radius * m_zoom;
        painter.drawEllipse(center, radius, radius);
    }
    else if (entity.type == VpEntityType::Arc)
    {
        const auto& arc = std::get<VpArcEntity>(entity.geometry);
        const QPointF center = worldToScreen(arc.center);
        const double radius = arc.radius * m_zoom;
        double span_angle =
            arc.is_clockwise ? arc.start_angle - arc.end_angle : arc.end_angle - arc.start_angle;
        if (span_angle <= 0.0)
        {
            span_angle += 360.0;
        }
        if (arc.is_clockwise)
        {
            span_angle = -span_angle;
        }
        painter.drawArc(
            QRectF(center.x() - radius, center.y() - radius, radius * 2.0, radius * 2.0),
            static_cast<int>(arc.start_angle * 16.0), static_cast<int>(span_angle * 16.0));
    }
    else if (entity.type == VpEntityType::Polyline)
    {
        const auto& polyline = std::get<VpPolylineEntity>(entity.geometry);
        if (polyline.vertices.size() < 2)
        {
            return;
        }
        const std::size_t segment_count =
            polyline.is_closed ? polyline.vertices.size() : polyline.vertices.size() - 1;
        for (std::size_t index = 0; index < segment_count; ++index)
        {
            const std::size_t end_index = (index + 1) % polyline.vertices.size();
            const std::vector<VpPoint2d> outline = polylineSegmentOutline(polyline, index);
            if (!outline.empty())
            {
                QPolygonF polygon;
                for (const VpPoint2d& point : outline)
                {
                    polygon.append(worldToScreen(point));
                }
                painter.save();
                painter.setBrush(painter.pen().color());
                painter.setPen(Qt::NoPen);
                painter.drawPolygon(polygon);
                painter.restore();
                continue;
            }
            VpArcEntity arc;
            if (index < polyline.bulges.size() &&
                bulgeArc(polyline.vertices[index], polyline.vertices[end_index],
                         polyline.bulges[index], arc))
            {
                VpEntityRecord arc_entity = entity;
                arc_entity.type = VpEntityType::Arc;
                arc_entity.geometry = arc;
                drawEntityGeometry(painter, arc_entity, fill_color);
            }
            else
            {
                painter.drawLine(worldToScreen(polyline.vertices[index]),
                                 worldToScreen(polyline.vertices[end_index]));
            }
        }
    }
    else if (entity.type == VpEntityType::Text)
    {
        const auto& text = std::get<VpTextEntity>(entity.geometry);
        painter.save();
        const QPointF position = worldToScreen(text.position);
        painter.translate(position);
        painter.rotate(-text.rotation);
        const QFont text_font =
            entityTextFont(m_document, text.style_name, text.height, m_zoom, painter.font());
        painter.setFont(text_font);
        const QRectF text_bounds = QFontMetricsF(text_font).boundingRect(text.text);
        double offset_x = 0.0;
        if (text.horizontal_alignment == VpTextHorizontalAlignment::Center)
        {
            offset_x = -text_bounds.width() * 0.5;
        }
        else if (text.horizontal_alignment == VpTextHorizontalAlignment::Right)
        {
            offset_x = -text_bounds.width();
        }
        double offset_y = 0.0;
        if (text.vertical_alignment == VpTextVerticalAlignment::Top)
        {
            offset_y = text_bounds.height();
        }
        else if (text.vertical_alignment == VpTextVerticalAlignment::Middle)
        {
            offset_y = text_bounds.height() * 0.5;
        }
        painter.drawText(QPointF(offset_x, offset_y), text.text);
        painter.restore();
    }
    else if (entity.type == VpEntityType::LinearDimension)
    {
        drawDimensionEntity(painter, std::get<VpLinearDimensionEntity>(entity.geometry));
    }
    else if (entity.type == VpEntityType::Hatch)
    {
        drawHatchEntity(painter, std::get<VpHatchEntity>(entity.geometry), fill_color);
    }
    else if (entity.type == VpEntityType::Spline)
    {
        const auto& spline = std::get<VpSplineEntity>(entity.geometry);
        QPainterPath path(worldToScreen(spline.control_points[0]));
        path.cubicTo(worldToScreen(spline.control_points[1]),
                     worldToScreen(spline.control_points[2]),
                     worldToScreen(spline.control_points[3]));
        painter.drawPath(path);
    }
    else if (entity.type == VpEntityType::Ellipse)
    {
        QPolygonF polygon;
        for (const VpPoint2d& point :
             ellipseApproximation(std::get<VpEllipseEntity>(entity.geometry)))
        {
            polygon.append(worldToScreen(point));
        }
        painter.drawPolyline(polygon);
    }
    else if (entity.type == VpEntityType::MText)
    {
        const auto& text = std::get<VpMTextEntity>(entity.geometry);
        painter.save();
        painter.translate(worldToScreen(text.position));
        painter.rotate(-text.rotation);
        QTextDocument document;
        document.setDefaultFont(
            entityTextFont(m_document, text.style_name, text.height, m_zoom, painter.font()));
        document.setDocumentMargin(0.0);
        document.setTextWidth(std::max(1.0, text.width * m_zoom));
        if (Qt::mightBeRichText(text.rich_text))
        {
            document.setHtml(text.rich_text);
        }
        else
        {
            document.setPlainText(text.rich_text);
        }
        double offset_x = 0.0;
        if (text.horizontal_alignment == VpTextHorizontalAlignment::Center)
        {
            offset_x = -document.idealWidth() * 0.5;
        }
        else if (text.horizontal_alignment == VpTextHorizontalAlignment::Right)
        {
            offset_x = -document.idealWidth();
        }
        painter.translate(offset_x, 0.0);
        document.drawContents(&painter);
        painter.restore();
    }
    else if (entity.type == VpEntityType::Leader)
    {
        const auto& leader = std::get<VpLeaderEntity>(entity.geometry);
        if (leader.vertices.size() < 2)
        {
            return;
        }
        QPolygonF path;
        for (const VpPoint2d& point : leader.vertices)
        {
            path.append(worldToScreen(point));
        }
        painter.drawPolyline(path);
        const QPointF arrow_tip = path.front();
        const QPointF direction = path[1] - arrow_tip;
        const double direction_length = std::hypot(direction.x(), direction.y());
        if (direction_length > 1.0e-9)
        {
            const QPointF unit = direction / direction_length;
            const QPointF normal(-unit.y(), unit.x());
            const double arrow_length = std::max(5.0, leader.arrow_size * m_zoom);
            QPolygonF arrow;
            arrow << arrow_tip << arrow_tip + (unit * arrow_length) + (normal * arrow_length * 0.4)
                  << arrow_tip + (unit * arrow_length) - (normal * arrow_length * 0.4);
            painter.save();
            painter.setBrush(painter.pen().color());
            painter.drawPolygon(arrow);
            painter.restore();
        }
        painter.save();
        painter.setFont(entityTextFont(m_document, leader.style_name, leader.text_height, m_zoom,
                                       painter.font()));
        painter.drawText(path.back() + QPointF(5.0, -4.0), leader.text);
        painter.restore();
    }
}

} // namespace Vp
