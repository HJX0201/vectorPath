#include "s_cad_viewport.h"

#include <QPainter>
#include <QPolygonF>
#include <cmath>

namespace vectorPath
{
namespace
{

QString objectSnapName(SObjectSnapType snap_type)
{
    switch (snap_type)
    {
    case SObjectSnapType::Endpoint:
        return QStringLiteral("端点");
    case SObjectSnapType::Midpoint:
        return QStringLiteral("中点");
    case SObjectSnapType::Center:
        return QStringLiteral("圆心");
    case SObjectSnapType::Quadrant:
        return QStringLiteral("象限点");
    case SObjectSnapType::Intersection:
        return QStringLiteral("交点");
    case SObjectSnapType::ApparentIntersection:
        return QStringLiteral("外观交点");
    case SObjectSnapType::Extension:
        return QStringLiteral("延伸");
    case SObjectSnapType::Parallel:
        return QStringLiteral("平行");
    case SObjectSnapType::Tracking:
        return QStringLiteral("对象追踪");
    case SObjectSnapType::Perpendicular:
        return QStringLiteral("垂足");
    case SObjectSnapType::Tangent:
        return QStringLiteral("切点");
    case SObjectSnapType::Nearest:
        return QStringLiteral("最近点");
    }
    return {};
}

} // namespace

void SCadViewport::drawObjectSnapMarker(QPainter& painter)
{
    if (!m_active_object_snap)
    {
        return;
    }
    const QPointF marker = worldToScreen(m_active_object_snap->point);
    const QColor snap_color(73, 224, 152);
    painter.save();
    if (m_active_object_snap->type == SObjectSnapType::Tracking)
    {
        std::vector<SPoint2d> anchors = m_tracking_points;
        if (m_first_point)
        {
            anchors.push_back(*m_first_point);
        }
        painter.setPen(QPen(QColor(73, 224, 152, 150), 1.0, Qt::DashLine));
        for (const SPoint2d& anchor : anchors)
        {
            if (std::abs(anchor.x - m_active_object_snap->point.x) <= 1.0e-7)
            {
                const double screen_x = worldToScreen(anchor).x();
                painter.drawLine(QPointF(screen_x, 0.0), QPointF(screen_x, height()));
            }
            if (std::abs(anchor.y - m_active_object_snap->point.y) <= 1.0e-7)
            {
                const double screen_y = worldToScreen(anchor).y();
                painter.drawLine(QPointF(0.0, screen_y), QPointF(width(), screen_y));
            }
        }
    }
    painter.setPen(QPen(snap_color, 1.6, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
    painter.setBrush(Qt::NoBrush);
    const QRectF marker_rect(marker.x() - 5.0, marker.y() - 5.0, 10.0, 10.0);
    if (m_active_object_snap->type == SObjectSnapType::Endpoint)
    {
        painter.drawRect(marker_rect);
    }
    else if (m_active_object_snap->type == SObjectSnapType::Midpoint)
    {
        QPolygonF triangle;
        triangle << QPointF(marker.x(), marker.y() - 6.0)
                 << QPointF(marker.x() - 6.0, marker.y() + 5.0)
                 << QPointF(marker.x() + 6.0, marker.y() + 5.0);
        painter.drawPolygon(triangle);
    }
    else if (m_active_object_snap->type == SObjectSnapType::Quadrant)
    {
        QPolygonF diamond;
        diamond << QPointF(marker.x(), marker.y() - 6.0) << QPointF(marker.x() + 6.0, marker.y())
                << QPointF(marker.x(), marker.y() + 6.0) << QPointF(marker.x() - 6.0, marker.y());
        painter.drawPolygon(diamond);
    }
    else if (m_active_object_snap->type == SObjectSnapType::Intersection ||
             m_active_object_snap->type == SObjectSnapType::ApparentIntersection)
    {
        painter.drawLine(marker + QPointF(-5.0, -5.0), marker + QPointF(5.0, 5.0));
        painter.drawLine(marker + QPointF(-5.0, 5.0), marker + QPointF(5.0, -5.0));
    }
    else if (m_active_object_snap->type == SObjectSnapType::Perpendicular)
    {
        painter.drawLine(marker + QPointF(-5.0, 5.0), marker + QPointF(-5.0, -5.0));
        painter.drawLine(marker + QPointF(-5.0, 5.0), marker + QPointF(5.0, 5.0));
        painter.drawLine(marker, marker + QPointF(0.0, 5.0));
        painter.drawLine(marker, marker + QPointF(-5.0, 0.0));
    }
    else if (m_active_object_snap->type == SObjectSnapType::Extension ||
             m_active_object_snap->type == SObjectSnapType::Parallel)
    {
        painter.drawLine(marker + QPointF(-7.0, 0.0), marker + QPointF(7.0, 0.0));
        painter.drawLine(marker + QPointF(-3.0, -3.0), marker + QPointF(3.0, 3.0));
        if (m_active_object_snap->type == SObjectSnapType::Parallel)
        {
            painter.drawLine(marker + QPointF(-7.0, 4.0), marker + QPointF(7.0, 4.0));
        }
    }
    else if (m_active_object_snap->type == SObjectSnapType::Tracking)
    {
        painter.drawLine(marker + QPointF(-6.0, 0.0), marker + QPointF(6.0, 0.0));
        painter.drawLine(marker + QPointF(0.0, -6.0), marker + QPointF(0.0, 6.0));
        painter.drawEllipse(marker, 2.5, 2.5);
    }
    else
    {
        painter.drawEllipse(marker_rect);
        if (m_active_object_snap->type == SObjectSnapType::Center)
        {
            painter.drawLine(marker + QPointF(-7.0, 0.0), marker + QPointF(7.0, 0.0));
            painter.drawLine(marker + QPointF(0.0, -7.0), marker + QPointF(0.0, 7.0));
        }
    }
    const QString snap_name = objectSnapName(m_active_object_snap->type);
    const QRectF text_rect(marker.x() + 10.0, marker.y() - 18.0, 52.0, 18.0);
    painter.fillRect(text_rect, QColor(15, 24, 29, 220));
    painter.setPen(snap_color);
    painter.drawText(text_rect.adjusted(4.0, 0.0, -2.0, 0.0), Qt::AlignVCenter, snap_name);
    painter.restore();
}

} // namespace vectorPath
