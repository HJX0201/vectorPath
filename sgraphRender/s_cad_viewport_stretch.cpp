#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_viewport_geometry.h"
#include "s_document_transaction.h"

#include <QPainter>
#include <algorithm>
#include <cmath>
#include <utility>

namespace smartGraphics
{
namespace
{

bool pointInsideWindow(const SPoint2d& point, const SPoint2d& first_corner,
                       const SPoint2d& second_corner)
{
    const double minimum_x = std::min(first_corner.x, second_corner.x);
    const double minimum_y = std::min(first_corner.y, second_corner.y);
    const double maximum_x = std::max(first_corner.x, second_corner.x);
    const double maximum_y = std::max(first_corner.y, second_corner.y);
    return point.x >= minimum_x && point.x <= maximum_x && point.y >= minimum_y &&
           point.y <= maximum_y;
}

} // namespace

bool stretchedEntity(const SEntityRecord& source, const SPoint2d& first_corner,
                     const SPoint2d& second_corner, double delta_x, double delta_y,
                     SEntityRecord& result)
{
    result = source;
    bool did_stretch = false;
    const auto stretch_point = [&](SPoint2d& point)
    {
        if (!pointInsideWindow(point, first_corner, second_corner))
        {
            return;
        }
        point.x += delta_x;
        point.y += delta_y;
        did_stretch = true;
    };

    if (result.type == SEntityType::Line)
    {
        auto& line = std::get<SLineEntity>(result.geometry);
        stretch_point(line.start_point);
        stretch_point(line.end_point);
    }
    else if (result.type == SEntityType::Polyline)
    {
        for (SPoint2d& vertex : std::get<SPolylineEntity>(result.geometry).vertices)
        {
            stretch_point(vertex);
        }
    }
    else if (result.type == SEntityType::Hatch)
    {
        auto& hatch = std::get<SHatchEntity>(result.geometry);
        for (SPoint2d& vertex : hatch.boundary)
        {
            stretch_point(vertex);
        }
        for (std::vector<SPoint2d>& island : hatch.island_boundaries)
        {
            for (SPoint2d& vertex : island)
            {
                stretch_point(vertex);
            }
        }
    }
    else if (result.type == SEntityType::LinearDimension)
    {
        auto& dimension = std::get<SLinearDimensionEntity>(result.geometry);
        stretch_point(dimension.first_point);
        stretch_point(dimension.second_point);
        stretch_point(dimension.dimension_line_point);
        stretch_point(dimension.center_point);
    }
    else
    {
        SPoint2d anchor;
        if (result.type == SEntityType::Circle)
        {
            anchor = std::get<SCircleEntity>(result.geometry).center;
        }
        else if (result.type == SEntityType::Arc)
        {
            anchor = std::get<SArcEntity>(result.geometry).center;
        }
        else if (result.type == SEntityType::Text)
        {
            anchor = std::get<STextEntity>(result.geometry).position;
        }
        else
        {
            return false;
        }
        if (pointInsideWindow(anchor, first_corner, second_corner))
        {
            result = translatedEntity(source, delta_x, delta_y);
            did_stretch = true;
        }
    }
    return did_stretch;
}

void SCadViewport::drawStretchPreview(QPainter& painter)
{
    if (m_input_points.empty())
    {
        return;
    }
    const SPoint2d opposite_corner =
        m_input_points.size() == 1 ? m_cursor_world : m_input_points[1];
    const QRectF window_rectangle(worldToScreen(m_input_points[0]), worldToScreen(opposite_corner));
    painter.save();
    painter.setPen(QPen(QColor(62, 156, 224), 1.2, Qt::DashLine));
    painter.setBrush(QColor(62, 156, 224, 38));
    painter.drawRect(window_rectangle.normalized());
    painter.restore();

    if (m_input_points.size() < 2 || !m_first_point)
    {
        return;
    }
    const double delta_x = m_cursor_world.x - m_first_point->x;
    const double delta_y = m_cursor_world.y - m_first_point->y;
    painter.setPen(QPen(QColor(255, 194, 92), 1.2, Qt::DashLine));
    painter.drawLine(worldToScreen(*m_first_point), worldToScreen(m_cursor_world));
    painter.setPen(QPen(QColor(92, 214, 255), 1.7, Qt::DashLine));
    for (SEntityId entity_id : m_selected_entity_ids)
    {
        const SEntityRecord* source = entityById(entity_id);
        SEntityRecord preview;
        if (source && stretchedEntity(*source, m_input_points[0], m_input_points[1], delta_x,
                                      delta_y, preview))
        {
            drawEntityGeometry(painter, preview, QColor(92, 214, 255, 72));
        }
    }
}

void SCadViewport::completeStretch(const SPoint2d& destination)
{
    if (!m_document || !m_first_point || m_input_points.size() < 2)
    {
        return;
    }
    const double delta_x = destination.x - m_first_point->x;
    const double delta_y = destination.y - m_first_point->y;
    if (std::abs(delta_x) <= 1.0e-9 && std::abs(delta_y) <= 1.0e-9)
    {
        emit commandMessage(tr("STRETCH 位移不能为零，请重新指定第二点："));
        return;
    }

    auto transaction = m_document->beginTransaction(tr("拉伸实体"));
    std::size_t stretched_count = 0;
    for (SEntityId entity_id : m_selected_entity_ids)
    {
        const SEntityRecord* source = entityById(entity_id);
        SEntityRecord replacement;
        if (source && stretchedEntity(*source, m_input_points[0], m_input_points[1], delta_x,
                                      delta_y, replacement))
        {
            transaction->replaceEntity(entity_id, std::move(replacement));
            ++stretched_count;
        }
    }
    transaction->commit();
    m_first_point.reset();
    m_input_points.clear();
    emit commandMessage(
        tr("STRETCH 已拉伸 %1 个实体。指定下一个交叉窗口第一个角点：").arg(stretched_count));
    update();
}

} // namespace smartGraphics
