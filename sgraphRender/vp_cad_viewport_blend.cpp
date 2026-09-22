#include "vp_cad_document.h"
#include "vp_cad_viewport.h"
#include "vp_cad_viewport_geometry.h"
#include "vp_document_transaction.h"
#include "vp_spline_geometry.h"

#include <QPainter>
#include <cmath>

namespace Vp
{
namespace
{

constexpr double kPi = 3.14159265358979323846;

struct VpBlendEndpoint
{
    VpPoint2d point;
    VpPoint2d outward_tangent;
};

bool normalizedVector(const VpPoint2d& vector, VpPoint2d& result)
{
    const double length = std::hypot(vector.x, vector.y);
    if (length <= 1.0e-9)
    {
        return false;
    }
    result = {vector.x / length, vector.y / length};
    return true;
}

VpPoint2d arcPoint(const VpArcEntity& arc, double angle)
{
    const double radians = angle * kPi / 180.0;
    return {arc.center.x + (arc.radius * std::cos(radians)),
            arc.center.y + (arc.radius * std::sin(radians))};
}

bool blendEndpoint(const VpEntityRecord& source, const VpPoint2d& pick_point,
                   VpBlendEndpoint& result)
{
    if (source.type == VpEntityType::Line)
    {
        const auto& line = std::get<VpLineEntity>(source.geometry);
        const bool use_start =
            distance(pick_point, line.start_point) <= distance(pick_point, line.end_point);
        result.point = use_start ? line.start_point : line.end_point;
        const VpPoint2d outward = use_start ? VpPoint2d{line.start_point.x - line.end_point.x,
                                                        line.start_point.y - line.end_point.y}
                                            : VpPoint2d{line.end_point.x - line.start_point.x,
                                                        line.end_point.y - line.start_point.y};
        return normalizedVector(outward, result.outward_tangent);
    }
    if (source.type == VpEntityType::Arc)
    {
        const auto& arc = std::get<VpArcEntity>(source.geometry);
        const VpPoint2d start_point = arcPoint(arc, arc.start_angle);
        const VpPoint2d end_point = arcPoint(arc, arc.end_angle);
        const bool use_start = distance(pick_point, start_point) <= distance(pick_point, end_point);
        const double angle = (use_start ? arc.start_angle : arc.end_angle) * kPi / 180.0;
        result.point = use_start ? start_point : end_point;
        result.outward_tangent = use_start ? VpPoint2d{std::sin(angle), -std::cos(angle)}
                                           : VpPoint2d{-std::sin(angle), std::cos(angle)};
        return true;
    }
    if (source.type == VpEntityType::Spline)
    {
        const auto& spline = std::get<VpSplineEntity>(source.geometry);
        const bool use_start = distance(pick_point, spline.control_points.front()) <=
                               distance(pick_point, spline.control_points.back());
        result.point = use_start ? spline.control_points.front() : spline.control_points.back();
        VpPoint2d tangent = splineTangent(spline, use_start ? 0.0 : 1.0);
        if (use_start)
        {
            tangent = {-tangent.x, -tangent.y};
        }
        return normalizedVector(tangent, result.outward_tangent);
    }
    return false;
}

bool isBlendCurve(const VpEntityRecord& entity)
{
    return entity.type == VpEntityType::Line || entity.type == VpEntityType::Arc ||
           entity.type == VpEntityType::Spline;
}

} // namespace

bool blendedSplineEntity(const VpEntityRecord& first_source, const VpEntityRecord& second_source,
                         const VpPoint2d& first_pick, const VpPoint2d& second_pick,
                         VpEntityRecord& result)
{
    if (first_source.id == second_source.id)
    {
        return false;
    }
    VpBlendEndpoint first_endpoint;
    VpBlendEndpoint second_endpoint;
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
    VpSplineEntity spline;
    spline.control_points = {
        first_endpoint.point,
        VpPoint2d{first_endpoint.point.x + (first_endpoint.outward_tangent.x * handle_length),
                  first_endpoint.point.y + (first_endpoint.outward_tangent.y * handle_length)},
        VpPoint2d{second_endpoint.point.x + (second_endpoint.outward_tangent.x * handle_length),
                  second_endpoint.point.y + (second_endpoint.outward_tangent.y * handle_length)},
        second_endpoint.point,
    };
    result = first_source;
    result.type = VpEntityType::Spline;
    result.geometry = spline;
    return true;
}

void VpCadViewport::acceptBlendPoint(const VpPoint2d& world_point)
{
    const std::optional<VpEntityId> picked_id = entityAt(world_point);
    const VpEntityRecord* picked_entity = picked_id ? entityById(*picked_id) : nullptr;
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
    const VpEntityRecord* first_source = entityById(*m_reference_entity_id);
    VpEntityRecord result;
    if (!first_source || m_input_points.empty() ||
        !blendedSplineEntity(*first_source, *picked_entity, m_input_points.front(), world_point,
                             result))
    {
        emit commandMessage(tr("BLEND 所选端点重合或曲线无法生成切向连续混接。"));
        return;
    }
    const VpEntityId first_id = first_source->id;
    const VpEntityId second_id = picked_entity->id;
    auto transaction = m_document->beginTransaction(tr("混接曲线"));
    const VpEntityId blend_id = transaction->addEntityCopy(std::move(result));
    transaction->commit();
    m_selected_entity_ids = {first_id, second_id, blend_id};
    m_selected_entity_id = blend_id;
    m_reference_entity_id.reset();
    m_input_points.clear();
    emitSelectionState();
    emit commandMessage(tr("BLEND 已创建切向连续三次样条曲线。继续选择第一条曲线："));
    update();
}

void VpCadViewport::drawBlendPreview(QPainter& painter)
{
    const std::optional<VpEntityId> hovered_id = entityAt(m_cursor_world);
    const VpEntityRecord* hovered = hovered_id ? entityById(*hovered_id) : nullptr;
    if (!m_reference_entity_id)
    {
        if (hovered && isBlendCurve(*hovered))
        {
            painter.setPen(QPen(QColor(105, 219, 142), 2.0, Qt::DashLine));
            drawEntityGeometry(painter, *hovered, QColor(105, 219, 142, 72));
        }
        return;
    }
    const VpEntityRecord* first_source = entityById(*m_reference_entity_id);
    VpEntityRecord preview;
    if (!first_source || !hovered || m_input_points.empty() ||
        !blendedSplineEntity(*first_source, *hovered, m_input_points.front(), m_cursor_world,
                             preview))
    {
        return;
    }
    painter.setPen(QPen(QColor(92, 214, 255), 2.2, Qt::DashLine));
    drawEntityGeometry(painter, preview, QColor(92, 214, 255, 72));
}

} // namespace Vp
