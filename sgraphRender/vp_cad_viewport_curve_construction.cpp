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

QString circlePrompt(VpCircleConstruction construction)
{
    switch (construction)
    {
    case VpCircleConstruction::CenterRadius:
        return QObject::tr("CIRCLE 圆心-半径：指定圆心：");
    case VpCircleConstruction::CenterDiameter:
        return QObject::tr("CIRCLE 圆心-直径：指定圆心：");
    case VpCircleConstruction::TwoPoint:
        return QObject::tr("CIRCLE 2P：指定直径第一个端点：");
    case VpCircleConstruction::ThreePoint:
        return QObject::tr("CIRCLE 3P：指定圆上第一点：");
    case VpCircleConstruction::TangentTangentRadius:
        return QObject::tr("CIRCLE TTR：选择第一条直线、圆或圆弧：");
    case VpCircleConstruction::TangentTangentTangent:
        return QObject::tr("CIRCLE TTT：选择第一条直线、圆或圆弧：");
    }
    return {};
}

QString arcPrompt(VpArcConstruction construction)
{
    switch (construction)
    {
    case VpArcConstruction::ThreePoint:
        return QObject::tr("ARC 3P：指定起点：");
    case VpArcConstruction::CenterStartEnd:
        return QObject::tr("ARC CSE：指定圆心：");
    case VpArcConstruction::StartCenterEnd:
        return QObject::tr("ARC SCE：指定起点：");
    case VpArcConstruction::StartCenterAngle:
        return QObject::tr("ARC SCA：指定起点：");
    case VpArcConstruction::CenterStartAngle:
        return QObject::tr("ARC CSA：指定圆心：");
    case VpArcConstruction::StartEndAngle:
        return QObject::tr("ARC SEA：指定起点：");
    case VpArcConstruction::StartEndDirection:
        return QObject::tr("ARC SED：指定起点：");
    case VpArcConstruction::StartEndRadius:
        return QObject::tr("ARC SER：指定起点：");
    }
    return {};
}

bool isParameterizedArcConstruction(VpArcConstruction construction)
{
    return construction == VpArcConstruction::StartCenterAngle ||
           construction == VpArcConstruction::CenterStartAngle ||
           construction == VpArcConstruction::StartEndAngle ||
           construction == VpArcConstruction::StartEndDirection ||
           construction == VpArcConstruction::StartEndRadius;
}

QString nextArcPrompt(VpArcConstruction construction, std::size_t point_count)
{
    if (construction == VpArcConstruction::ThreePoint)
    {
        return point_count == 1 ? QObject::tr("ARC 3P：指定弧上第二点：")
                                : QObject::tr("ARC 3P：指定端点：");
    }
    if (construction == VpArcConstruction::CenterStartEnd)
    {
        return point_count == 1 ? QObject::tr("ARC CSE：指定起点：")
                                : QObject::tr("ARC CSE：指定端点方向：");
    }
    if (construction == VpArcConstruction::StartCenterEnd)
    {
        return point_count == 1 ? QObject::tr("ARC SCE：指定圆心：")
                                : QObject::tr("ARC SCE：指定端点方向：");
    }
    if (construction == VpArcConstruction::StartCenterAngle)
    {
        return point_count == 1 ? QObject::tr("ARC SCA：指定圆心：")
                                : QObject::tr("ARC SCA：输入包含角，或在视口指定终止方向：");
    }
    if (construction == VpArcConstruction::CenterStartAngle)
    {
        return point_count == 1 ? QObject::tr("ARC CSA：指定起点：")
                                : QObject::tr("ARC CSA：输入包含角，或在视口指定终止方向：");
    }
    if (construction == VpArcConstruction::StartEndAngle)
    {
        return point_count == 1 ? QObject::tr("ARC SEA：指定端点：")
                                : QObject::tr("ARC SEA：输入包含角，或在视口指定弧上点：");
    }
    if (construction == VpArcConstruction::StartEndDirection)
    {
        return point_count == 1 ? QObject::tr("ARC SED：指定端点：")
                                : QObject::tr("ARC SED：输入起点切线方向角，或指定方向点：");
    }
    return point_count == 1 ? QObject::tr("ARC SER：指定端点：")
                            : QObject::tr("ARC SER：输入半径，或在视口指定圆心侧和半径：");
}

bool arcFromInputs(VpArcConstruction construction, const std::vector<VpPoint2d>& points,
                   const std::optional<double>& parameter, VpArcEntity& result)
{
    if (construction == VpArcConstruction::ThreePoint)
    {
        return points.size() >= 3 &&
               calculateThreePointArc(points[0], points[1], points[2], result.center, result.radius,
                                      result.start_angle, result.end_angle);
    }
    if (construction == VpArcConstruction::CenterStartEnd ||
        construction == VpArcConstruction::StartCenterEnd)
    {
        if (points.size() < 3)
        {
            return false;
        }
        result.center = construction == VpArcConstruction::CenterStartEnd ? points[0] : points[1];
        const VpPoint2d start =
            construction == VpArcConstruction::CenterStartEnd ? points[1] : points[0];
        result.radius = distance(result.center, start);
        result.start_angle = entityAngleDegrees(result.center, start);
        result.end_angle = entityAngleDegrees(result.center, points[2]);
        return result.radius > 1.0e-9 && distance(result.center, points[2]) > 1.0e-9 &&
               std::abs(std::remainder(result.end_angle - result.start_angle, 360.0)) > 1.0e-7;
    }
    if (points.size() < 2)
    {
        return false;
    }
    if (construction == VpArcConstruction::StartCenterAngle ||
        construction == VpArcConstruction::CenterStartAngle)
    {
        const VpPoint2d center =
            construction == VpArcConstruction::CenterStartAngle ? points[0] : points[1];
        const VpPoint2d start =
            construction == VpArcConstruction::CenterStartAngle ? points[1] : points[0];
        double included_angle = parameter.value_or(0.0);
        if (!parameter)
        {
            if (points.size() < 3)
            {
                return false;
            }
            included_angle =
                entityAngleDegrees(center, points[2]) - entityAngleDegrees(center, start);
            if (included_angle <= 0.0)
            {
                included_angle += 360.0;
            }
        }
        return arcFromCenterStartAngle(center, start, included_angle, result);
    }
    if (construction == VpArcConstruction::StartEndAngle)
    {
        if (parameter)
        {
            return arcFromStartEndAngle(points[0], points[1], *parameter, result);
        }
        return points.size() >= 3 &&
               calculateThreePointArc(points[0], points[2], points[1], result.center, result.radius,
                                      result.start_angle, result.end_angle);
    }
    if (construction == VpArcConstruction::StartEndDirection)
    {
        if (!parameter && points.size() < 3)
        {
            return false;
        }
        const double direction = parameter ? *parameter : entityAngleDegrees(points[0], points[2]);
        return arcFromStartEndDirection(points[0], points[1], direction, result);
    }
    if (!parameter && points.size() < 3)
    {
        return false;
    }
    const VpPoint2d chord_midpoint{(points[0].x + points[1].x) * 0.5,
                                   (points[0].y + points[1].y) * 0.5};
    const double radius = parameter ? *parameter
                                    : std::max(distance(points[0], points[1]) * 0.5,
                                               distance(chord_midpoint, points[2]));
    double side_sign = 1.0;
    if (!parameter)
    {
        const VpPoint2d chord{points[1].x - points[0].x, points[1].y - points[0].y};
        const VpPoint2d side{points[2].x - chord_midpoint.x, points[2].y - chord_midpoint.y};
        side_sign = ((chord.x * side.y) - (chord.y * side.x)) < 0.0 ? -1.0 : 1.0;
    }
    return arcFromStartEndRadius(points[0], points[1], radius, side_sign, result);
}

} // namespace

void VpCadViewport::setCircleConstruction(VpCircleConstruction construction)
{
    setToolMode(VpToolMode::Circle);
    m_circle_construction = construction;
    m_first_point.reset();
    m_input_points.clear();
    m_curve_reference_ids.clear();
    emit commandMessage(circlePrompt(construction));
    update();
}

void VpCadViewport::setCircleTangentRadius(double radius)
{
    if (!std::isfinite(radius) || radius <= 1.0e-9 || radius > 1.0e9)
    {
        emit commandMessage(tr("CIRCLE TTR 半径必须大于零。"));
        return;
    }
    m_circle_tangent_radius = radius;
    setCircleConstruction(VpCircleConstruction::TangentTangentRadius);
    emit commandMessage(
        tr("CIRCLE TTR 半径 %1。选择第一条直线、圆或圆弧：").arg(radius, 0, 'f', 3));
}

void VpCadViewport::setArcConstruction(VpArcConstruction construction)
{
    setToolMode(VpToolMode::Arc);
    m_arc_construction = construction;
    m_arc_construction_parameter.reset();
    m_first_point.reset();
    m_input_points.clear();
    emit commandMessage(arcPrompt(construction));
    update();
}

void VpCadViewport::setArcConstructionParameter(VpArcConstruction construction, double parameter)
{
    setArcConstruction(construction);
    if (!isParameterizedArcConstruction(construction) || !std::isfinite(parameter))
    {
        emit commandMessage(tr("ARC 构造参数无效。"));
        return;
    }
    m_arc_construction_parameter = parameter;
    emit commandMessage(
        tr("ARC 参数 %1。%2").arg(parameter, 0, 'f', 3).arg(arcPrompt(construction)));
}

void VpCadViewport::acceptCircleConstructionPoint(const VpPoint2d& world_point)
{
    if (m_circle_construction == VpCircleConstruction::TangentTangentRadius ||
        m_circle_construction == VpCircleConstruction::TangentTangentTangent)
    {
        acceptTangentCirclePoint(world_point);
        return;
    }
    m_input_points.push_back(world_point);
    const std::size_t required_count =
        m_circle_construction == VpCircleConstruction::ThreePoint ? 3U : 2U;
    if (m_input_points.size() < required_count)
    {
        if (m_circle_construction == VpCircleConstruction::ThreePoint)
        {
            emit commandMessage(tr("CIRCLE 3P：指定圆上第 %1 点：").arg(m_input_points.size() + 1));
        }
        else if (m_circle_construction == VpCircleConstruction::TwoPoint)
        {
            emit commandMessage(tr("CIRCLE 2P：指定直径第二个端点："));
        }
        else if (m_circle_construction == VpCircleConstruction::CenterDiameter)
        {
            emit commandMessage(tr("CIRCLE 指定直径："));
        }
        else
        {
            emit commandMessage(tr("CIRCLE 指定半径："));
        }
        update();
        return;
    }

    VpPoint2d center;
    double radius = 0.0;
    if (m_circle_construction == VpCircleConstruction::ThreePoint)
    {
        double start_angle = 0.0;
        double end_angle = 0.0;
        calculateThreePointArc(m_input_points[0], m_input_points[1], m_input_points[2], center,
                               radius, start_angle, end_angle);
    }
    else if (m_circle_construction == VpCircleConstruction::TwoPoint)
    {
        center = {(m_input_points[0].x + m_input_points[1].x) * 0.5,
                  (m_input_points[0].y + m_input_points[1].y) * 0.5};
        radius = distance(m_input_points[0], m_input_points[1]) * 0.5;
    }
    else
    {
        center = m_input_points[0];
        radius = distance(center, m_input_points[1]);
        if (m_circle_construction == VpCircleConstruction::CenterDiameter)
        {
            radius *= 0.5;
        }
    }
    if (radius > 1.0e-9)
    {
        auto transaction = m_document->beginTransaction(tr("创建圆"));
        transaction->addCircle(center, radius);
        transaction->commit();
    }
    else
    {
        emit commandMessage(tr("CIRCLE 输入点不能构成有效圆。"));
    }
    m_input_points.clear();
    emit commandMessage(circlePrompt(m_circle_construction));
    update();
}

bool VpCadViewport::tangentCirclePreview(const VpPoint2d& candidate_pick,
                                         VpCircleEntity& result) const
{
    const std::optional<VpEntityId> candidate_id = entityAt(candidate_pick);
    if (!candidate_id || std::find(m_curve_reference_ids.begin(), m_curve_reference_ids.end(),
                                   *candidate_id) != m_curve_reference_ids.end())
    {
        return false;
    }
    const VpEntityRecord* candidate = entityById(*candidate_id);
    if (!candidate)
    {
        return false;
    }
    if (m_circle_construction == VpCircleConstruction::TangentTangentRadius &&
        m_curve_reference_ids.size() == 1 && m_input_points.size() == 1)
    {
        const VpEntityRecord* first = entityById(m_curve_reference_ids.front());
        return first && tangentCircleToTwoEntities(*first, *candidate, m_input_points.front(),
                                                   candidate_pick, m_circle_tangent_radius, result);
    }
    if (m_circle_construction == VpCircleConstruction::TangentTangentTangent &&
        m_curve_reference_ids.size() == 2 && m_input_points.size() == 2)
    {
        const VpEntityRecord* first = entityById(m_curve_reference_ids[0]);
        const VpEntityRecord* second = entityById(m_curve_reference_ids[1]);
        if (!first || !second)
        {
            return false;
        }
        return tangentCircleToThreeEntities({*first, *second, *candidate},
                                            {m_input_points[0], m_input_points[1], candidate_pick},
                                            result);
    }
    return false;
}

void VpCadViewport::acceptTangentCirclePoint(const VpPoint2d& world_point)
{
    const std::optional<VpEntityId> picked_id = entityAt(world_point);
    const VpEntityRecord* picked = picked_id ? entityById(*picked_id) : nullptr;
    const bool is_ttt = m_circle_construction == VpCircleConstruction::TangentTangentTangent;
    const bool is_supported =
        picked && (picked->type == VpEntityType::Line || picked->type == VpEntityType::Circle ||
                   picked->type == VpEntityType::Arc);
    if (!is_supported)
    {
        emit commandMessage(is_ttt ? tr("CIRCLE TTT 请选择直线、圆或圆弧。")
                                   : tr("CIRCLE TTR 请选择直线、圆或圆弧。"));
        return;
    }
    if (std::find(m_curve_reference_ids.begin(), m_curve_reference_ids.end(), *picked_id) !=
        m_curve_reference_ids.end())
    {
        emit commandMessage(tr("CIRCLE 相切构造不能重复选择同一实体。"));
        return;
    }
    m_curve_reference_ids.push_back(*picked_id);
    m_input_points.push_back(world_point);
    m_selected_entity_ids = m_curve_reference_ids;
    m_selected_entity_id = m_selected_entity_ids.front();
    emitSelectionState();
    const std::size_t required_count = is_ttt ? 3U : 2U;
    if (m_curve_reference_ids.size() < required_count)
    {
        emit commandMessage(is_ttt ? tr("CIRCLE TTT：选择第 %1 条直线、圆或圆弧：")
                                         .arg(m_curve_reference_ids.size() + 1)
                                   : tr("CIRCLE TTR 半径 %1：选择第二条直线、圆或圆弧：")
                                         .arg(m_circle_tangent_radius, 0, 'f', 3));
        update();
        return;
    }

    VpCircleEntity circle;
    bool is_valid = false;
    if (is_ttt)
    {
        const VpEntityRecord* first = entityById(m_curve_reference_ids[0]);
        const VpEntityRecord* second = entityById(m_curve_reference_ids[1]);
        const VpEntityRecord* third = entityById(m_curve_reference_ids[2]);
        is_valid = first && second && third &&
                   tangentCircleToThreeEntities(
                       {*first, *second, *third},
                       {m_input_points[0], m_input_points[1], m_input_points[2]}, circle);
    }
    else
    {
        const VpEntityRecord* first = entityById(m_curve_reference_ids[0]);
        const VpEntityRecord* second = entityById(m_curve_reference_ids[1]);
        is_valid = first && second &&
                   tangentCircleToTwoEntities(*first, *second, m_input_points[0], m_input_points[1],
                                              m_circle_tangent_radius, circle);
    }
    if (is_valid)
    {
        auto transaction = m_document->beginTransaction(tr("创建相切圆"));
        transaction->addCircle(circle.center, circle.radius);
        transaction->commit();
    }
    else
    {
        emit commandMessage(tr("CIRCLE 所选对象没有符合拾取侧的有效相切圆。"));
    }
    m_curve_reference_ids.clear();
    m_input_points.clear();
    m_selected_entity_ids.clear();
    m_selected_entity_id.reset();
    emitSelectionState();
    emit commandMessage(circlePrompt(m_circle_construction));
    update();
}

void VpCadViewport::acceptArcConstructionPoint(const VpPoint2d& world_point)
{
    m_input_points.push_back(world_point);
    const std::size_t required_count =
        isParameterizedArcConstruction(m_arc_construction) && m_arc_construction_parameter ? 2U
                                                                                           : 3U;
    if (m_input_points.size() < required_count)
    {
        emit commandMessage(nextArcPrompt(m_arc_construction, m_input_points.size()));
        update();
        return;
    }

    completeArcConstruction();
}

void VpCadViewport::completeArcConstruction()
{
    VpArcEntity arc;
    if (m_document &&
        arcFromInputs(m_arc_construction, m_input_points, m_arc_construction_parameter, arc))
    {
        auto transaction = m_document->beginTransaction(tr("创建圆弧"));
        transaction->addArc(arc.center, arc.radius, arc.start_angle, arc.end_angle);
        transaction->commit();
    }
    else
    {
        emit commandMessage(tr("ARC 输入点不能构成有效圆弧。"));
    }
    m_input_points.clear();
    emit commandMessage(arcPrompt(m_arc_construction));
    update();
}

void VpCadViewport::drawCircleConstructionPreview(QPainter& painter)
{
    if (m_circle_construction == VpCircleConstruction::TangentTangentRadius ||
        m_circle_construction == VpCircleConstruction::TangentTangentTangent)
    {
        VpCircleEntity circle;
        if (tangentCirclePreview(m_cursor_world, circle))
        {
            painter.drawEllipse(worldToScreen(circle.center), circle.radius * m_zoom,
                                circle.radius * m_zoom);
        }
        return;
    }
    if (m_input_points.empty())
    {
        return;
    }
    VpPoint2d center = m_input_points[0];
    double radius = 0.0;
    if (m_circle_construction == VpCircleConstruction::ThreePoint && m_input_points.size() >= 2)
    {
        double start_angle = 0.0;
        double end_angle = 0.0;
        if (!calculateThreePointArc(m_input_points[0], m_input_points[1], m_cursor_world, center,
                                    radius, start_angle, end_angle))
        {
            painter.drawLine(worldToScreen(m_input_points[0]), worldToScreen(m_input_points[1]));
            painter.drawLine(worldToScreen(m_input_points[1]), worldToScreen(m_cursor_world));
            return;
        }
    }
    else if (m_circle_construction == VpCircleConstruction::TwoPoint)
    {
        center = {(m_input_points[0].x + m_cursor_world.x) * 0.5,
                  (m_input_points[0].y + m_cursor_world.y) * 0.5};
        radius = distance(m_input_points[0], m_cursor_world) * 0.5;
    }
    else if (m_circle_construction == VpCircleConstruction::ThreePoint)
    {
        painter.drawLine(worldToScreen(m_input_points[0]), worldToScreen(m_cursor_world));
        return;
    }
    else
    {
        radius = distance(center, m_cursor_world);
        if (m_circle_construction == VpCircleConstruction::CenterDiameter)
        {
            radius *= 0.5;
        }
    }
    painter.drawEllipse(worldToScreen(center), radius * m_zoom, radius * m_zoom);
}

void VpCadViewport::drawArcConstructionPreview(QPainter& painter)
{
    if (m_input_points.empty())
    {
        return;
    }
    std::vector<VpPoint2d> preview_points = m_input_points;
    preview_points.push_back(m_cursor_world);
    VpArcEntity arc;
    if (!arcFromInputs(m_arc_construction, preview_points, m_arc_construction_parameter, arc))
    {
        painter.drawLine(worldToScreen(m_input_points.back()), worldToScreen(m_cursor_world));
        return;
    }
    double span_angle = arc.end_angle - arc.start_angle;
    if (span_angle <= 0.0)
    {
        span_angle += 360.0;
    }
    const QPointF screen_center = worldToScreen(arc.center);
    const double screen_radius = arc.radius * m_zoom;
    painter.drawArc(QRectF(screen_center.x() - screen_radius, screen_center.y() - screen_radius,
                           screen_radius * 2.0, screen_radius * 2.0),
                    static_cast<int>(arc.start_angle * 16.0), static_cast<int>(span_angle * 16.0));
}

} // namespace Vp
