#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_viewport_geometry.h"
#include "s_document_transaction.h"
#include "s_spline_geometry.h"

#include <QPainter>
#include <cmath>

namespace smartGraphics
{
namespace
{

constexpr double kPi = 3.14159265358979323846;

struct SBlendEndpoint
{
    SPoint2d point;
    SPoint2d outward_tangent;
};

bool normalizedVector(const SPoint2d& vector, SPoint2d& result)
{
    const double length = std::hypot(vector.x, vector.y);
    if (length <= 1.0e-9)
    {
        return false;
    }
    result = {vector.x / length, vector.y / length};
    return true;
}

SPoint2d arcPoint(const SArcEntity& arc, double angle)
{
    const double radians = angle * kPi / 180.0;
    return {arc.center.x + (arc.radius * std::cos(radians)),
            arc.center.y + (arc.radius * std::sin(radians))};
}

bool blendEndpoint(const SEntityRecord& source, const SPoint2d& pick_point, SBlendEndpoint& result)
{
    if (source.type == SEntityType::Line)
    {
        const auto& line = std::get<SLineEntity>(source.geometry);
        const bool use_start =
            distance(pick_point, line.start_point) <= distance(pick_point, line.end_point);
        result.point = use_start ? line.start_point : line.end_point;
        const SPoint2d outward = use_start ? SPoint2d{line.start_point.x - line.end_point.x,
                                                      line.start_point.y - line.end_point.y}
                                           : SPoint2d{line.end_point.x - line.start_point.x,
                                                      line.end_point.y - line.start_point.y};
        return normalizedVector(outward, result.outward_tangent);
    }
    if (source.type == SEntityType::Arc)
    {
        const auto& arc = std::get<SArcEntity>(source.geometry);
        const SPoint2d start_point = arcPoint(arc, arc.start_angle);
        const SPoint2d end_point = arcPoint(arc, arc.end_angle);
        const bool use_start = distance(pick_point, start_point) <= distance(pick_point, end_point);
        const double angle = (use_start ? arc.start_angle : arc.end_angle) * kPi / 180.0;
        result.point = use_start ? start_point : end_point;
        result.outward_tangent = use_start ? SPoint2d{std::sin(angle), -std::cos(angle)}
                                           : SPoint2d{-std::sin(angle), std::cos(angle)};
        return true;
    }
    if (source.type == SEntityType::Spline)
    {
        const auto& spline = std::get<SSplineEntity>(source.geometry);
        const bool use_start = distance(pick_point, spline.control_points.front()) <=
                               distance(pick_point, spline.control_points.back());
        result.point = use_start ? spline.control_points.front() : spline.control_points.back();
        SPoint2d tangent = splineTangent(spline, use_start ? 0.0 : 1.0);
        if (use_start)
        {
            tangent = {-tangent.x, -tangent.y};
        }
        return normalizedVector(tangent, result.outward_tangent);
    }
    return false;
}

bool isBlendCurve(const SEntityRecord& entity)
{
    return entity.type == SEntityType::Line || entity.type == SEntityType::Arc ||
           entity.type == SEntityType::Spline;
}

} // namespace

bool blendedSplineEntity(const SEntityRecord& first_source, const SEntityRecord& second_source,
                         const SPoint2d& first_pick, const SPoint2d& second_pick,
                         SEntityRecord& result)
{
    if (first_source.id == second_source.id)
    {
        return false;
    }
    SBlendEndpoint first_endpoint;
    SBlendEndpoint second_endpoint;
    if (!blendEndpoint(first_source, first_pick, first_endpoint) ||
        !blendEndpoint(second_source, second_pick, second_endpoint))
    {
        return false;
    }
    const double chord_length = distance(first_endpoint.point, second_endpoint.point);
    if (chord_length <= 1.0e-9)
    {
        return false;
    }
    const double handle_length = chord_length / 3.0;
    SSplineEntity spline;
    spline.control_points = {
        first_endpoint.point,
        SPoint2d{first_endpoint.point.x + (first_endpoint.outward_tangent.x * handle_length),
                 first_endpoint.point.y + (first_endpoint.outward_tangent.y * handle_length)},
        SPoint2d{second_endpoint.point.x + (second_endpoint.outward_tangent.x * handle_length),
                 second_endpoint.point.y + (second_endpoint.outward_tangent.y * handle_length)},
        second_endpoint.point,
    };
    result = first_source;
    result.type = SEntityType::Spline;
    result.geometry = spline;
    return true;
}

void SCadViewport::acceptBlendPoint(const SPoint2d& world_point)
{
    const std::optional<SEntityId> picked_id = entityAt(world_point);
    const SEntityRecord* picked_entity = picked_id ? entityById(*picked_id) : nullptr;
    if (!picked_entity || !isBlendCurve(*picked_entity))
    {
        emit commandMessage(tr("BLEND 请选择直线、圆弧或样条曲线。"));
        return;
    }
    if (!m_reference_entity_id)
    {
        m_reference_entity_id = picked_entity->id;
        m_input_points = {world_point};
        m_selected_entity_ids = {picked_entity->id};
        m_selected_entity_id = picked_entity->id;
        emitSelectionState();
        emit commandMessage(tr("BLEND 在第二条曲线的待连接端部附近拾取："));
        update();
        return;
    }
    const SEntityRecord* first_source = entityById(*m_reference_entity_id);
    SEntityRecord result;
    if (!first_source || m_input_points.empty() ||
        !blendedSplineEntity(*first_source, *picked_entity, m_input_points.front(), world_point,
                             result))
    {
        emit commandMessage(tr("BLEND 所选端点重合或曲线无法生成切向连续混接。"));
        return;
    }
    const SEntityId first_id = first_source->id;
    const SEntityId second_id = picked_entity->id;
    auto transaction = m_document->beginTransaction(tr("混接曲线"));
    const SEntityId blend_id = transaction->addEntityCopy(std::move(result));
    transaction->commit();
    m_selected_entity_ids = {first_id, second_id, blend_id};
    m_selected_entity_id = blend_id;
    m_reference_entity_id.reset();
    m_input_points.clear();
    emitSelectionState();
    emit commandMessage(tr("BLEND 已创建切向连续三次样条曲线。继续选择第一条曲线："));
    update();
}

void SCadViewport::drawBlendPreview(QPainter& painter)
{
    const std::optional<SEntityId> hovered_id = entityAt(m_cursor_world);
    const SEntityRecord* hovered = hovered_id ? entityById(*hovered_id) : nullptr;
    if (!m_reference_entity_id)
    {
        if (hovered && isBlendCurve(*hovered))
        {
            painter.setPen(QPen(QColor(105, 219, 142), 2.0, Qt::DashLine));
            drawEntityGeometry(painter, *hovered, QColor(105, 219, 142, 72));
        }
        return;
    }
    const SEntityRecord* first_source = entityById(*m_reference_entity_id);
    SEntityRecord preview;
    if (!first_source || !hovered || m_input_points.empty() ||
        !blendedSplineEntity(*first_source, *hovered, m_input_points.front(), m_cursor_world,
                             preview))
    {
        return;
    }
    painter.setPen(QPen(QColor(92, 214, 255), 2.2, Qt::DashLine));
    drawEntityGeometry(painter, preview, QColor(92, 214, 255, 72));
}

} // namespace smartGraphics
