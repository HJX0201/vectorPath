#include "vp_cad_document.h"
#include "vp_cad_viewport.h"
#include "vp_cad_viewport_geometry.h"
#include "vp_document_transaction.h"

#include <QPainter>
#include <algorithm>
#include <cmath>
#include <utility>

namespace Vp
{
namespace
{

constexpr double kPi = 3.14159265358979323846;

bool infiniteLineIntersection(const VpLineEntity& first_line, const VpLineEntity& second_line,
                              VpPoint2d& intersection)
{
    const double first_x = first_line.end_point.x - first_line.start_point.x;
    const double first_y = first_line.end_point.y - first_line.start_point.y;
    const double second_x = second_line.end_point.x - second_line.start_point.x;
    const double second_y = second_line.end_point.y - second_line.start_point.y;
    const double denominator = (first_x * second_y) - (first_y * second_x);
    if (std::abs(denominator) <= 1.0e-12)
    {
        return false;
    }
    const double origin_x = second_line.start_point.x - first_line.start_point.x;
    const double origin_y = second_line.start_point.y - first_line.start_point.y;
    const double first_parameter = ((origin_x * second_y) - (origin_y * second_x)) / denominator;
    intersection = {first_line.start_point.x + (first_parameter * first_x),
                    first_line.start_point.y + (first_parameter * first_y)};
    return true;
}

bool pickedDirection(const VpLineEntity& line, const VpPoint2d& intersection,
                     const VpPoint2d& pick_point, VpPoint2d& direction, VpPoint2d& retained_point)
{
    const double line_x = line.end_point.x - line.start_point.x;
    const double line_y = line.end_point.y - line.start_point.y;
    const double line_length = std::hypot(line_x, line_y);
    if (line_length <= 1.0e-12)
    {
        return false;
    }
    direction = {line_x / line_length, line_y / line_length};
    double pick_projection = ((pick_point.x - intersection.x) * direction.x) +
                             ((pick_point.y - intersection.y) * direction.y);
    if (std::abs(pick_projection) <= 1.0e-9)
    {
        const double start_projection = ((line.start_point.x - intersection.x) * direction.x) +
                                        ((line.start_point.y - intersection.y) * direction.y);
        const double end_projection = ((line.end_point.x - intersection.x) * direction.x) +
                                      ((line.end_point.y - intersection.y) * direction.y);
        pick_projection = std::abs(start_projection) >= std::abs(end_projection) ? start_projection
                                                                                 : end_projection;
    }
    if (pick_projection < 0.0)
    {
        direction.x = -direction.x;
        direction.y = -direction.y;
    }
    const double start_distance = ((line.start_point.x - intersection.x) * direction.x) +
                                  ((line.start_point.y - intersection.y) * direction.y);
    const double end_distance = ((line.end_point.x - intersection.x) * direction.x) +
                                ((line.end_point.y - intersection.y) * direction.y);
    retained_point = start_distance >= end_distance ? line.start_point : line.end_point;
    return true;
}

} // namespace

bool filletedLineEntities(const VpEntityRecord& first_source, const VpEntityRecord& second_source,
                          const VpPoint2d& first_pick, const VpPoint2d& second_pick, double radius,
                          VpEntityRecord& first_result, VpEntityRecord& second_result,
                          VpEntityRecord& arc_result)
{
    if (first_source.type != VpEntityType::Line || second_source.type != VpEntityType::Line ||
        first_source.id == second_source.id || radius <= 1.0e-9)
    {
        return false;
    }
    const auto& first_line = std::get<VpLineEntity>(first_source.geometry);
    const auto& second_line = std::get<VpLineEntity>(second_source.geometry);
    VpPoint2d intersection;
    if (!infiniteLineIntersection(first_line, second_line, intersection))
    {
        return false;
    }
    VpPoint2d first_direction;
    VpPoint2d second_direction;
    VpPoint2d first_retained;
    VpPoint2d second_retained;
    if (!pickedDirection(first_line, intersection, first_pick, first_direction, first_retained) ||
        !pickedDirection(second_line, intersection, second_pick, second_direction, second_retained))
    {
        return false;
    }

    const double direction_dot = std::clamp((first_direction.x * second_direction.x) +
                                                (first_direction.y * second_direction.y),
                                            -1.0, 1.0);
    const double included_angle = std::acos(direction_dot);
    if (included_angle <= 1.0e-6 || included_angle >= kPi - 1.0e-6)
    {
        return false;
    }
    const double tangent_distance = radius / std::tan(included_angle * 0.5);
    const double center_distance = radius / std::sin(included_angle * 0.5);
    const double bisector_x = first_direction.x + second_direction.x;
    const double bisector_y = first_direction.y + second_direction.y;
    const double bisector_length = std::hypot(bisector_x, bisector_y);
    if (!std::isfinite(tangent_distance) || bisector_length <= 1.0e-12)
    {
        return false;
    }

    const VpPoint2d first_tangent{intersection.x + (first_direction.x * tangent_distance),
                                  intersection.y + (first_direction.y * tangent_distance)};
    const VpPoint2d second_tangent{intersection.x + (second_direction.x * tangent_distance),
                                   intersection.y + (second_direction.y * tangent_distance)};
    const VpPoint2d center{intersection.x + ((bisector_x / bisector_length) * center_distance),
                           intersection.y + ((bisector_y / bisector_length) * center_distance)};
    first_result = first_source;
    first_result.geometry = VpLineEntity{first_retained, first_tangent};
    second_result = second_source;
    second_result.geometry = VpLineEntity{second_retained, second_tangent};
    double start_angle = entityAngleDegrees(center, first_tangent);
    double end_angle = entityAngleDegrees(center, second_tangent);
    double counter_clockwise_span = std::fmod(end_angle - start_angle + 360.0, 360.0);
    if (counter_clockwise_span > 180.0)
    {
        std::swap(start_angle, end_angle);
    }
    arc_result = first_source;
    arc_result.type = VpEntityType::Arc;
    arc_result.geometry = VpArcEntity{center, radius, start_angle, end_angle};
    return true;
}

void VpCadViewport::setFilletRadius(double radius)
{
    if (radius <= 1.0e-9 || !std::isfinite(radius))
    {
        emit commandMessage(tr("FILLET 半径必须是大于零的有限数值。"));
        return;
    }
    m_fillet_radius = radius;
    emit commandMessage(
        tr("FILLET 半径已设为 %1。选择直线、圆、圆弧或多段线：").arg(m_fillet_radius, 0, 'f', 2));
    update();
}

double VpCadViewport::filletRadius() const noexcept
{
    return m_fillet_radius;
}

void VpCadViewport::acceptFilletPoint(const VpPoint2d& world_point)
{
    const std::optional<VpEntityId> picked_id = entityAt(world_point);
    const VpEntityRecord* picked_entity = picked_id ? entityById(*picked_id) : nullptr;
    if (!picked_entity ||
        (picked_entity->type != VpEntityType::Line && picked_entity->type != VpEntityType::Circle &&
         picked_entity->type != VpEntityType::Arc && picked_entity->type != VpEntityType::Polyline))
    {
        emit commandMessage(tr("FILLET 请选择直线、圆、圆弧或多段线。"));
        return;
    }
    if (!m_reference_entity_id && picked_entity->type == VpEntityType::Polyline)
    {
        VpEntityRecord result;
        if (!filletedPolylineEntity(*picked_entity, m_fillet_radius, result))
        {
            emit commandMessage(tr("FILLET 多段线含既有弧段、尖角或边长不足。"));
            return;
        }
        auto transaction = m_document->beginTransaction(tr("多段线圆角"));
        transaction->replaceEntity(picked_entity->id, std::move(result));
        transaction->commit();
        m_selected_entity_ids = {picked_entity->id};
        m_selected_entity_id = picked_entity->id;
        emitSelectionState();
        emit commandMessage(tr("FILLET 已处理多段线全部角点。继续选择："));
        update();
        return;
    }
    if (!m_reference_entity_id)
    {
        m_reference_entity_id = picked_entity->id;
        m_input_points = {world_point};
        m_selected_entity_ids = {picked_entity->id};
        m_selected_entity_id = picked_entity->id;
        emitSelectionState();
        emit commandMessage(tr("FILLET 选择第二个可相切实体："));
        update();
        return;
    }
    const VpEntityRecord* first_source = entityById(*m_reference_entity_id);
    VpEntityRecord first_result;
    VpEntityRecord second_result;
    VpEntityRecord arc_result;
    if (!first_source || picked_entity->id == first_source->id || m_input_points.empty() ||
        !filletedEntities(*first_source, *picked_entity, m_input_points.front(), world_point,
                          m_fillet_radius, first_result, second_result, arc_result))
    {
        emit commandMessage(tr("FILLET 所选实体无法按当前半径和拾取侧生成圆角。"));
        return;
    }
    const VpEntityId first_id = first_source->id;
    const VpEntityId second_id = picked_entity->id;
    auto transaction = m_document->beginTransaction(tr("实体圆角"));
    transaction->replaceEntity(first_id, std::move(first_result));
    transaction->replaceEntity(second_id, std::move(second_result));
    const VpEntityId arc_id = transaction->addEntityCopy(std::move(arc_result));
    transaction->commit();
    m_selected_entity_ids = {first_id, second_id, arc_id};
    m_selected_entity_id = first_id;
    m_reference_entity_id.reset();
    m_input_points.clear();
    emitSelectionState();
    emit commandMessage(
        tr("FILLET 已创建半径 %1 的圆角。继续选择第一个实体：").arg(m_fillet_radius, 0, 'f', 2));
    update();
}

void VpCadViewport::drawFilletPreview(QPainter& painter)
{
    const std::optional<VpEntityId> hovered_id = entityAt(m_cursor_world);
    const VpEntityRecord* hovered = hovered_id ? entityById(*hovered_id) : nullptr;
    if (!m_reference_entity_id)
    {
        if (hovered && hovered->type == VpEntityType::Polyline)
        {
            VpEntityRecord preview;
            if (filletedPolylineEntity(*hovered, m_fillet_radius, preview))
            {
                painter.setPen(QPen(QColor(92, 214, 255), 2.0, Qt::DashLine));
                drawEntityGeometry(painter, preview, QColor(92, 214, 255, 72));
            }
        }
        else if (hovered &&
                 (hovered->type == VpEntityType::Line || hovered->type == VpEntityType::Circle ||
                  hovered->type == VpEntityType::Arc))
        {
            painter.setPen(QPen(QColor(105, 219, 142), 2.0, Qt::DashLine));
            drawEntityGeometry(painter, *hovered, QColor(105, 219, 142, 72));
        }
        return;
    }
    const VpEntityRecord* first_source = entityById(*m_reference_entity_id);
    VpEntityRecord first_result;
    VpEntityRecord second_result;
    VpEntityRecord arc_result;
    if (!first_source || !hovered || m_input_points.empty() ||
        !filletedEntities(*first_source, *hovered, m_input_points.front(), m_cursor_world,
                          m_fillet_radius, first_result, second_result, arc_result))
    {
        return;
    }
    painter.setPen(QPen(QColor(92, 214, 255), 1.7, Qt::DashLine));
    drawEntityGeometry(painter, first_result, QColor(92, 214, 255, 72));
    drawEntityGeometry(painter, second_result, QColor(92, 214, 255, 72));
    painter.setPen(QPen(QColor(105, 219, 142), 2.2, Qt::SolidLine));
    drawEntityGeometry(painter, arc_result, QColor(105, 219, 142, 72));
}

} // namespace Vp
