#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_viewport_geometry.h"
#include "s_document_transaction.h"

#include <QPainter>
#include <cmath>
#include <utility>

namespace vectorPath
{
namespace
{

bool lineCornerRays(const SEntityRecord& first_source, const SEntityRecord& second_source,
                    const SPoint2d& first_pick, const SPoint2d& second_pick, SPoint2d& intersection,
                    SPoint2d& first_direction, SPoint2d& second_direction, SPoint2d& first_retained,
                    SPoint2d& second_retained)
{
    if (first_source.type != SEntityType::Line || second_source.type != SEntityType::Line ||
        first_source.id == second_source.id)
    {
        return false;
    }
    const auto& first_line = std::get<SLineEntity>(first_source.geometry);
    const auto& second_line = std::get<SLineEntity>(second_source.geometry);
    const double first_x = first_line.end_point.x - first_line.start_point.x;
    const double first_y = first_line.end_point.y - first_line.start_point.y;
    const double second_x = second_line.end_point.x - second_line.start_point.x;
    const double second_y = second_line.end_point.y - second_line.start_point.y;
    const double denominator = (first_x * second_y) - (first_y * second_x);
    const double first_length = std::hypot(first_x, first_y);
    const double second_length = std::hypot(second_x, second_y);
    if (std::abs(denominator) <= 1.0e-12 || first_length <= 1.0e-12 || second_length <= 1.0e-12)
    {
        return false;
    }
    const double origin_x = second_line.start_point.x - first_line.start_point.x;
    const double origin_y = second_line.start_point.y - first_line.start_point.y;
    const double parameter = ((origin_x * second_y) - (origin_y * second_x)) / denominator;
    intersection = {first_line.start_point.x + (parameter * first_x),
                    first_line.start_point.y + (parameter * first_y)};
    first_direction = {first_x / first_length, first_y / first_length};
    second_direction = {second_x / second_length, second_y / second_length};
    const auto orient_direction = [&intersection](const SLineEntity& line,
                                                  const SPoint2d& pick_point, SPoint2d& direction,
                                                  SPoint2d& retained)
    {
        double pick_projection = ((pick_point.x - intersection.x) * direction.x) +
                                 ((pick_point.y - intersection.y) * direction.y);
        if (std::abs(pick_projection) <= 1.0e-9)
        {
            pick_projection = ((line.end_point.x - intersection.x) * direction.x) +
                              ((line.end_point.y - intersection.y) * direction.y);
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
        retained = start_distance >= end_distance ? line.start_point : line.end_point;
    };
    orient_direction(first_line, first_pick, first_direction, first_retained);
    orient_direction(second_line, second_pick, second_direction, second_retained);
    const double direction_cross =
        (first_direction.x * second_direction.y) - (first_direction.y * second_direction.x);
    return std::abs(direction_cross) > 1.0e-9;
}

} // namespace

bool chamferedLineEntities(const SEntityRecord& first_source, const SEntityRecord& second_source,
                           const SPoint2d& first_pick, const SPoint2d& second_pick,
                           double first_distance, double second_distance,
                           SEntityRecord& first_result, SEntityRecord& second_result,
                           SEntityRecord& chamfer_result)
{
    if (first_distance <= 1.0e-9 || second_distance <= 1.0e-9)
    {
        return false;
    }
    SPoint2d intersection;
    SPoint2d first_direction;
    SPoint2d second_direction;
    SPoint2d first_retained;
    SPoint2d second_retained;
    if (!lineCornerRays(first_source, second_source, first_pick, second_pick, intersection,
                        first_direction, second_direction, first_retained, second_retained))
    {
        return false;
    }
    const SPoint2d first_chamfer{intersection.x + (first_direction.x * first_distance),
                                 intersection.y + (first_direction.y * first_distance)};
    const SPoint2d second_chamfer{intersection.x + (second_direction.x * second_distance),
                                  intersection.y + (second_direction.y * second_distance)};
    first_result = first_source;
    first_result.geometry = SLineEntity{first_retained, first_chamfer};
    second_result = second_source;
    second_result.geometry = SLineEntity{second_retained, second_chamfer};
    chamfer_result = first_source;
    chamfer_result.geometry = SLineEntity{first_chamfer, second_chamfer};
    return distance(first_chamfer, second_chamfer) > 1.0e-9;
}

bool chamferedPolylineEntity(const SEntityRecord& source, double first_distance,
                             double second_distance, SEntityRecord& result)
{
    if (source.type != SEntityType::Polyline || first_distance <= 1.0e-9 ||
        second_distance <= 1.0e-9)
    {
        return false;
    }
    const auto& polyline = std::get<SPolylineEntity>(source.geometry);
    if (std::any_of(polyline.bulges.begin(), polyline.bulges.end(),
                    [](double bulge)
                    {
                        return std::abs(bulge) > 1.0e-12;
                    }))
    {
        return false;
    }
    const std::size_t vertex_count = polyline.vertices.size();
    if (vertex_count < 3)
    {
        return false;
    }
    const std::size_t segment_count = polyline.is_closed ? vertex_count : vertex_count - 1;
    for (std::size_t index = 0; index < segment_count; ++index)
    {
        const std::size_t end_index = (index + 1) % vertex_count;
        const double start_trim = (polyline.is_closed || index > 0) ? second_distance : 0.0;
        const double end_trim =
            (polyline.is_closed || end_index + 1 < vertex_count) ? first_distance : 0.0;
        if (distance(polyline.vertices[index], polyline.vertices[end_index]) <=
            start_trim + end_trim + 1.0e-9)
        {
            return false;
        }
    }
    const std::size_t first_corner = polyline.is_closed ? 0U : 1U;
    const std::size_t corner_end = polyline.is_closed ? vertex_count : vertex_count - 1;
    std::vector<SPoint2d> vertices;
    vertices.reserve(vertex_count * 2);
    if (!polyline.is_closed)
    {
        vertices.push_back(polyline.vertices.front());
    }
    for (std::size_t index = first_corner; index < corner_end; ++index)
    {
        const SPoint2d& previous = polyline.vertices[(index + vertex_count - 1) % vertex_count];
        const SPoint2d& corner = polyline.vertices[index];
        const SPoint2d& next = polyline.vertices[(index + 1) % vertex_count];
        const double incoming_length = distance(previous, corner);
        const double outgoing_length = distance(corner, next);
        if (incoming_length <= first_distance + 1.0e-9 ||
            outgoing_length <= second_distance + 1.0e-9)
        {
            return false;
        }
        vertices.push_back(
            {corner.x + ((previous.x - corner.x) * first_distance / incoming_length),
             corner.y + ((previous.y - corner.y) * first_distance / incoming_length)});
        vertices.push_back({corner.x + ((next.x - corner.x) * second_distance / outgoing_length),
                            corner.y + ((next.y - corner.y) * second_distance / outgoing_length)});
    }
    if (!polyline.is_closed)
    {
        vertices.push_back(polyline.vertices.back());
    }
    result = source;
    result.geometry = SPolylineEntity{std::move(vertices), polyline.is_closed};
    return true;
}

void SCadViewport::setChamferDistances(double first_distance, double second_distance)
{
    if (first_distance <= 1.0e-9 || second_distance <= 1.0e-9 || !std::isfinite(first_distance) ||
        !std::isfinite(second_distance))
    {
        emit commandMessage(tr("CHAMFER 两个距离都必须是大于零的有限数值。"));
        return;
    }
    m_chamfer_first_distance = first_distance;
    m_chamfer_second_distance = second_distance;
    emit commandMessage(tr("CHAMFER 距离已设为 %1, %2。选择直线、圆弧或整条多段线：")
                            .arg(first_distance, 0, 'f', 2)
                            .arg(second_distance, 0, 'f', 2));
    update();
}

void SCadViewport::acceptChamferPoint(const SPoint2d& world_point)
{
    const std::optional<SEntityId> picked_id = entityAt(world_point);
    const SEntityRecord* picked_entity = picked_id ? entityById(*picked_id) : nullptr;
    if (!picked_entity ||
        (picked_entity->type != SEntityType::Line && picked_entity->type != SEntityType::Arc &&
         picked_entity->type != SEntityType::Polyline))
    {
        emit commandMessage(tr("CHAMFER 请选择直线、圆弧或多段线。"));
        return;
    }
    if (!m_reference_entity_id && picked_entity->type == SEntityType::Polyline)
    {
        SEntityRecord result;
        if (!chamferedPolylineEntity(*picked_entity, m_chamfer_first_distance,
                                     m_chamfer_second_distance, result))
        {
            emit commandMessage(tr("CHAMFER 多段线边长不足，无法按当前距离处理全部角点。"));
            return;
        }
        auto transaction = m_document->beginTransaction(tr("多段线倒角"));
        transaction->replaceEntity(picked_entity->id, std::move(result));
        transaction->commit();
        m_selected_entity_ids = {picked_entity->id};
        m_selected_entity_id = picked_entity->id;
        emitSelectionState();
        emit commandMessage(tr("CHAMFER 已处理多段线全部角点。继续选择："));
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
        emit commandMessage(tr("CHAMFER 选择第二个直线或圆弧实体："));
        update();
        return;
    }
    const SEntityRecord* first_source = entityById(*m_reference_entity_id);
    SEntityRecord first_result;
    SEntityRecord second_result;
    SEntityRecord chamfer_result;
    if (!first_source || picked_entity->id == first_source->id || m_input_points.empty() ||
        !chamferedEntities(*first_source, *picked_entity, m_input_points.front(), world_point,
                           m_chamfer_first_distance, m_chamfer_second_distance, first_result,
                           second_result, chamfer_result))
    {
        emit commandMessage(tr("CHAMFER 所选实体不相交或无法按当前距离和拾取侧倒角。"));
        return;
    }
    const SEntityId first_id = first_source->id;
    const SEntityId second_id = picked_entity->id;
    auto transaction = m_document->beginTransaction(tr("实体倒角"));
    transaction->replaceEntity(first_id, std::move(first_result));
    transaction->replaceEntity(second_id, std::move(second_result));
    const SEntityId chamfer_id = transaction->addEntityCopy(std::move(chamfer_result));
    transaction->commit();
    m_selected_entity_ids = {first_id, second_id, chamfer_id};
    m_selected_entity_id = first_id;
    m_reference_entity_id.reset();
    m_input_points.clear();
    emitSelectionState();
    emit commandMessage(tr("CHAMFER 已创建倒角。继续选择第一条直线："));
    update();
}

void SCadViewport::drawChamferPreview(QPainter& painter)
{
    const std::optional<SEntityId> hovered_id = entityAt(m_cursor_world);
    const SEntityRecord* hovered = hovered_id ? entityById(*hovered_id) : nullptr;
    if (!m_reference_entity_id)
    {
        if (hovered && hovered->type == SEntityType::Polyline)
        {
            SEntityRecord preview;
            if (chamferedPolylineEntity(*hovered, m_chamfer_first_distance,
                                        m_chamfer_second_distance, preview))
            {
                painter.setPen(QPen(QColor(92, 214, 255), 2.0, Qt::DashLine));
                drawEntityGeometry(painter, preview, QColor(92, 214, 255, 72));
            }
        }
        else if (hovered &&
                 (hovered->type == SEntityType::Line || hovered->type == SEntityType::Arc))
        {
            painter.setPen(QPen(QColor(105, 219, 142), 2.0, Qt::DashLine));
            drawEntityGeometry(painter, *hovered, QColor(105, 219, 142, 72));
        }
        return;
    }
    const SEntityRecord* first_source = entityById(*m_reference_entity_id);
    SEntityRecord first_result;
    SEntityRecord second_result;
    SEntityRecord chamfer_result;
    if (!first_source || !hovered || m_input_points.empty() ||
        !chamferedEntities(*first_source, *hovered, m_input_points.front(), m_cursor_world,
                           m_chamfer_first_distance, m_chamfer_second_distance, first_result,
                           second_result, chamfer_result))
    {
        return;
    }
    painter.setPen(QPen(QColor(92, 214, 255), 1.7, Qt::DashLine));
    drawEntityGeometry(painter, first_result, QColor(92, 214, 255, 72));
    drawEntityGeometry(painter, second_result, QColor(92, 214, 255, 72));
    painter.setPen(QPen(QColor(105, 219, 142), 2.2, Qt::SolidLine));
    drawEntityGeometry(painter, chamfer_result, QColor(105, 219, 142, 72));
}

} // namespace vectorPath
