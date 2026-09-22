#include "vp_cad_document.h"
#include "vp_cad_viewport.h"
#include "vp_cad_viewport_geometry.h"
#include "vp_document_transaction.h"

#include <QPainter>
#include <cmath>

namespace Vp
{
namespace
{

constexpr double kPi = 3.14159265358979323846;

double normalizedAngle(double angle)
{
    const double result = std::fmod(angle, 360.0);
    return result < 0.0 ? result + 360.0 : result;
}

bool lengthenedLine(const VpLineEntity& source, bool edit_start, const VpPoint2d& destination,
                    VpLineEntity& result)
{
    const VpPoint2d fixed_point = edit_start ? source.end_point : source.start_point;
    const VpPoint2d edited_point = edit_start ? source.start_point : source.end_point;
    const double direction_x = edited_point.x - fixed_point.x;
    const double direction_y = edited_point.y - fixed_point.y;
    const double source_length = std::hypot(direction_x, direction_y);
    if (source_length <= 1.0e-12)
    {
        return false;
    }
    const double unit_x = direction_x / source_length;
    const double unit_y = direction_y / source_length;
    const double target_length =
        ((destination.x - fixed_point.x) * unit_x) + ((destination.y - fixed_point.y) * unit_y);
    if (target_length <= 1.0e-9)
    {
        return false;
    }
    const VpPoint2d target{fixed_point.x + (unit_x * target_length),
                           fixed_point.y + (unit_y * target_length)};
    result = source;
    (edit_start ? result.start_point : result.end_point) = target;
    return true;
}

VpPoint2d pointOnCircle(const VpPoint2d& center, double radius, const VpPoint2d& direction_point)
{
    const double angle = entityAngleDegrees(center, direction_point) * kPi / 180.0;
    return {center.x + (radius * std::cos(angle)), center.y + (radius * std::sin(angle))};
}

double updatedBulge(const VpPoint2d& center, const VpPoint2d& start_point,
                    const VpPoint2d& end_point, double source_bulge)
{
    const double start_angle = entityAngleDegrees(center, start_point);
    const double end_angle = entityAngleDegrees(center, end_point);
    const double sweep = source_bulge > 0.0 ? normalizedAngle(end_angle - start_angle)
                                            : -normalizedAngle(start_angle - end_angle);
    return std::tan((sweep * kPi / 180.0) * 0.25);
}

bool lengthenedPolyline(const VpPolylineEntity& source, bool edit_start,
                        const VpPoint2d& destination, VpPolylineEntity& result)
{
    if (source.is_closed || source.vertices.size() < 2)
    {
        return false;
    }
    result = source;
    result.bulges.resize(result.vertices.size(), 0.0);
    const std::size_t segment_index = edit_start ? 0 : source.vertices.size() - 2;
    const double bulge = segment_index < source.bulges.size() ? source.bulges[segment_index] : 0.0;
    if (std::abs(bulge) <= 1.0e-12)
    {
        VpLineEntity line{source.vertices[segment_index], source.vertices[segment_index + 1]};
        VpLineEntity edited;
        if (!lengthenedLine(line, edit_start, destination, edited))
        {
            return false;
        }
        result.vertices[segment_index] = edited.start_point;
        result.vertices[segment_index + 1] = edited.end_point;
        return true;
    }
    VpArcEntity arc;
    if (!bulgeArc(source.vertices[segment_index], source.vertices[segment_index + 1], bulge, arc))
    {
        return false;
    }
    const VpPoint2d target = pointOnCircle(arc.center, arc.radius, destination);
    if (edit_start)
    {
        result.vertices.front() = target;
    }
    else
    {
        result.vertices.back() = target;
    }
    const double new_bulge = updatedBulge(arc.center, result.vertices[segment_index],
                                          result.vertices[segment_index + 1], bulge);
    if (std::abs(new_bulge) <= 1.0e-9 || !std::isfinite(new_bulge))
    {
        return false;
    }
    result.bulges[segment_index] = new_bulge;
    return true;
}

} // namespace

bool lengthenedEntity(const VpEntityRecord& source, const VpPoint2d& pick_point,
                      const VpPoint2d& destination, VpEntityRecord& result)
{
    result = source;
    if (source.type == VpEntityType::Line)
    {
        const auto& line = std::get<VpLineEntity>(source.geometry);
        const bool edit_start =
            distance(pick_point, line.start_point) <= distance(pick_point, line.end_point);
        VpLineEntity edited;
        if (!lengthenedLine(line, edit_start, destination, edited))
        {
            return false;
        }
        result.geometry = edited;
        return true;
    }
    if (source.type == VpEntityType::Arc)
    {
        const auto& arc = std::get<VpArcEntity>(source.geometry);
        const VpPoint2d start_point =
            pointOnCircle(arc.center, arc.radius,
                          {arc.center.x + std::cos(arc.start_angle * kPi / 180.0),
                           arc.center.y + std::sin(arc.start_angle * kPi / 180.0)});
        const VpPoint2d end_point =
            pointOnCircle(arc.center, arc.radius,
                          {arc.center.x + std::cos(arc.end_angle * kPi / 180.0),
                           arc.center.y + std::sin(arc.end_angle * kPi / 180.0)});
        auto edited = arc;
        if (distance(pick_point, start_point) <= distance(pick_point, end_point))
        {
            edited.start_angle = entityAngleDegrees(arc.center, destination);
        }
        else
        {
            edited.end_angle = entityAngleDegrees(arc.center, destination);
        }
        if (normalizedAngle(edited.end_angle - edited.start_angle) <= 1.0e-7)
        {
            return false;
        }
        result.geometry = edited;
        return true;
    }
    if (source.type == VpEntityType::Polyline)
    {
        const auto& polyline = std::get<VpPolylineEntity>(source.geometry);
        const bool edit_start =
            !polyline.vertices.empty() && distance(pick_point, polyline.vertices.front()) <=
                                              distance(pick_point, polyline.vertices.back());
        VpPolylineEntity edited;
        if (!lengthenedPolyline(polyline, edit_start, destination, edited))
        {
            return false;
        }
        result.geometry = std::move(edited);
        return true;
    }
    return false;
}

void VpCadViewport::acceptLengthenPoint(const VpPoint2d& world_point)
{
    if (!m_reference_entity_id)
    {
        const std::optional<VpEntityId> picked_id = entityAt(world_point);
        const VpEntityRecord* picked = picked_id ? entityById(*picked_id) : nullptr;
        const bool supported =
            picked && (picked->type == VpEntityType::Line || picked->type == VpEntityType::Arc ||
                       (picked->type == VpEntityType::Polyline &&
                        !std::get<VpPolylineEntity>(picked->geometry).is_closed));
        if (!supported)
        {
            emit commandMessage(tr("LENGTHEN 请选择直线、圆弧或开放多段线的端部："));
            return;
        }
        m_reference_entity_id = picked->id;
        m_first_point = world_point;
        m_selected_entity_ids = {picked->id};
        m_selected_entity_id = picked->id;
        emitSelectionState();
        emit commandMessage(tr("LENGTHEN 移动光标预览，指定新的端点位置："));
        update();
        return;
    }
    const VpEntityRecord* source = entityById(*m_reference_entity_id);
    VpEntityRecord result;
    if (!source || !m_first_point ||
        !lengthenedEntity(*source, *m_first_point, world_point, result))
    {
        emit commandMessage(tr("LENGTHEN 新位置会产生无效长度，请重新指定："));
        return;
    }
    auto transaction = m_document->beginTransaction(tr("动态拉长"));
    transaction->replaceEntity(source->id, std::move(result));
    transaction->commit();
    m_reference_entity_id.reset();
    m_first_point.reset();
    emit commandMessage(tr("LENGTHEN 完成。继续选择另一个端部，Esc 结束："));
    update();
}

void VpCadViewport::drawLengthenPreview(QPainter& painter)
{
    if (!m_reference_entity_id)
    {
        const std::optional<VpEntityId> hovered_id = entityAt(m_cursor_world);
        const VpEntityRecord* hovered = hovered_id ? entityById(*hovered_id) : nullptr;
        if (hovered && (hovered->type == VpEntityType::Line || hovered->type == VpEntityType::Arc ||
                        (hovered->type == VpEntityType::Polyline &&
                         !std::get<VpPolylineEntity>(hovered->geometry).is_closed)))
        {
            painter.setPen(QPen(QColor(105, 219, 142), 2.0, Qt::DashLine));
            drawEntityGeometry(painter, *hovered, QColor(105, 219, 142, 48));
        }
        return;
    }
    const VpEntityRecord* source = entityById(*m_reference_entity_id);
    VpEntityRecord preview;
    if (source && m_first_point &&
        lengthenedEntity(*source, *m_first_point, m_cursor_world, preview))
    {
        painter.setPen(QPen(QColor(92, 214, 255), 1.8, Qt::DashLine));
        drawEntityGeometry(painter, preview, QColor(92, 214, 255, 48));
    }
}

} // namespace Vp
