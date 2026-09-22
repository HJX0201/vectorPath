#include "vp_cad_document.h"
#include "vp_dimension_geometry.h"
#include "vp_document_transaction.h"
#include "vp_hatch_geometry.h"

#include <QSet>
#include <algorithm>
#include <cmath>

namespace Vp
{
namespace
{

bool validPoint(const VpPoint2d& point) noexcept
{
    return std::isfinite(point.x) && std::isfinite(point.y);
}

bool expectedGeometry(const VpEntityRecord& entity) noexcept
{
    switch (entity.type)
    {
    case VpEntityType::Line:
        return std::holds_alternative<VpLineEntity>(entity.geometry);
    case VpEntityType::Circle:
        return std::holds_alternative<VpCircleEntity>(entity.geometry);
    case VpEntityType::Arc:
        return std::holds_alternative<VpArcEntity>(entity.geometry);
    case VpEntityType::Polyline:
        return std::holds_alternative<VpPolylineEntity>(entity.geometry);
    case VpEntityType::Text:
        return std::holds_alternative<VpTextEntity>(entity.geometry);
    case VpEntityType::LinearDimension:
        return std::holds_alternative<VpLinearDimensionEntity>(entity.geometry);
    case VpEntityType::Hatch:
        return std::holds_alternative<VpHatchEntity>(entity.geometry);
    case VpEntityType::Spline:
        return std::holds_alternative<VpSplineEntity>(entity.geometry);
    case VpEntityType::Ellipse:
        return std::holds_alternative<VpEllipseEntity>(entity.geometry);
    case VpEntityType::MText:
        return std::holds_alternative<VpMTextEntity>(entity.geometry);
    case VpEntityType::Leader:
        return std::holds_alternative<VpLeaderEntity>(entity.geometry);
    }
    return false;
}

bool validGeometry(const VpEntityRecord& entity) noexcept
{
    if (!expectedGeometry(entity))
    {
        return false;
    }
    if (entity.type == VpEntityType::Line)
    {
        const auto& line = std::get<VpLineEntity>(entity.geometry);
        return validPoint(line.start_point) && validPoint(line.end_point) &&
               distance(line.start_point, line.end_point) > 1.0e-12;
    }
    if (entity.type == VpEntityType::Circle)
    {
        const auto& circle = std::get<VpCircleEntity>(entity.geometry);
        return validPoint(circle.center) && std::isfinite(circle.radius) && circle.radius > 1.0e-12;
    }
    if (entity.type == VpEntityType::Arc)
    {
        const auto& arc = std::get<VpArcEntity>(entity.geometry);
        return validPoint(arc.center) && std::isfinite(arc.radius) && arc.radius > 1.0e-12 &&
               std::isfinite(arc.start_angle) && std::isfinite(arc.end_angle);
    }
    if (entity.type == VpEntityType::Polyline)
    {
        const auto& polyline = std::get<VpPolylineEntity>(entity.geometry);
        const std::size_t minimum_size = polyline.is_closed ? 3U : 2U;
        return polyline.vertices.size() >= minimum_size &&
               std::all_of(polyline.vertices.begin(), polyline.vertices.end(), validPoint) &&
               std::all_of(polyline.bulges.begin(), polyline.bulges.end(),
                           [](double value)
                           {
                               return std::isfinite(value);
                           });
    }
    if (entity.type == VpEntityType::Text)
    {
        const auto& text = std::get<VpTextEntity>(entity.geometry);
        return validPoint(text.position) && !text.text.isEmpty() && std::isfinite(text.height) &&
               text.height > 1.0e-12 && std::isfinite(text.rotation);
    }
    if (entity.type == VpEntityType::LinearDimension)
    {
        return isDimensionValid(std::get<VpLinearDimensionEntity>(entity.geometry));
    }
    if (entity.type == VpEntityType::Hatch)
    {
        return isHatchValid(std::get<VpHatchEntity>(entity.geometry));
    }
    if (entity.type == VpEntityType::Spline)
    {
        const auto& spline = std::get<VpSplineEntity>(entity.geometry);
        return std::all_of(spline.control_points.begin(), spline.control_points.end(),
                           validPoint) &&
               distance(spline.control_points.front(), spline.control_points.back()) > 1.0e-12;
    }
    if (entity.type == VpEntityType::Ellipse)
    {
        const auto& ellipse = std::get<VpEllipseEntity>(entity.geometry);
        return validPoint(ellipse.center) && validPoint(ellipse.major_axis) &&
               validPoint(ellipse.minor_axis) &&
               std::hypot(ellipse.major_axis.x, ellipse.major_axis.y) > 1.0e-12 &&
               std::hypot(ellipse.minor_axis.x, ellipse.minor_axis.y) > 1.0e-12;
    }
    if (entity.type == VpEntityType::MText)
    {
        const auto& text = std::get<VpMTextEntity>(entity.geometry);
        return validPoint(text.position) && !text.rich_text.isEmpty() &&
               std::isfinite(text.width) && text.width > 1.0e-12 && std::isfinite(text.height) &&
               text.height > 1.0e-12 && std::isfinite(text.rotation);
    }
    if (entity.type == VpEntityType::Leader)
    {
        const auto& leader = std::get<VpLeaderEntity>(entity.geometry);
        return leader.vertices.size() >= 2 &&
               std::all_of(leader.vertices.begin(), leader.vertices.end(), validPoint) &&
               !leader.text.isEmpty() && std::isfinite(leader.text_height) &&
               leader.text_height > 1.0e-12 && std::isfinite(leader.arrow_size) &&
               leader.arrow_size > 1.0e-12;
    }
    return false;
}

void addIssue(VpDocumentAuditReport& report, VpAuditSeverity severity, VpEntityId entity_id,
              const QString& message, bool was_repaired)
{
    report.issues.push_back({severity, entity_id, message, was_repaired});
    ++report.issue_count;
}

} // namespace

VpDocumentAuditReport VpCadDocument::audit(bool repair)
{
    VpDocumentAuditReport report;
    report.scanned_entity_count = static_cast<int>(m_entities.size());
    QSet<quint64> entity_ids;
    auto transaction = beginTransaction(tr("AUDIT 修复图形"));
    bool has_repairs = false;
    for (const VpEntityRecord& entity : m_entities)
    {
        if (entity.id == 0 || entity_ids.contains(static_cast<quint64>(entity.id)))
        {
            addIssue(report, VpAuditSeverity::Error, entity.id,
                     tr("实体 ID 为零或与其他实体重复；为避免误删，该问题需要重新导入修复。"),
                     false);
            continue;
        }
        entity_ids.insert(static_cast<quint64>(entity.id));
        if (!validGeometry(entity))
        {
            if (repair)
            {
                transaction->removeEntity(entity.id);
                ++report.removed_entity_count;
                has_repairs = true;
            }
            addIssue(report, VpAuditSeverity::Error, entity.id, tr("实体几何无效。"), repair);
            continue;
        }
        VpEntityRecord replacement = entity;
        QStringList repairs;
        if (!layer(entity.layer_name))
        {
            replacement.layer_name =
                layer(QStringLiteral("0")) ? QStringLiteral("0") : m_current_layer_name;
            repairs.append(tr("无效图层已重定向"));
        }
        if (!std::isfinite(entity.line_width_mm) || entity.line_width_mm < -1.0)
        {
            replacement.line_width_mm = -1.0;
            repairs.append(tr("线宽已恢复为随层"));
        }
        if (entity.type == VpEntityType::Polyline)
        {
            auto& polyline = std::get<VpPolylineEntity>(replacement.geometry);
            const std::size_t vertex_count = polyline.vertices.size();
            if (polyline.bulges.size() != vertex_count ||
                polyline.start_widths.size() != vertex_count ||
                polyline.end_widths.size() != vertex_count)
            {
                polyline.bulges.resize(vertex_count, 0.0);
                polyline.start_widths.resize(vertex_count, 0.0);
                polyline.end_widths.resize(vertex_count, 0.0);
                repairs.append(tr("多段线段属性数组已对齐"));
            }
        }
        if (entity.associative_array &&
            (!expectedGeometry(VpEntityRecord{0,
                                              entity.associative_array->source_type,
                                              {},
                                              -1.0,
                                              entity.associative_array->source_geometry,
                                              {}}) ||
             entity.associative_array->array_id == 0))
        {
            replacement.associative_array.reset();
            repairs.append(tr("无效关联阵列数据已移除"));
        }
        if (repairs.isEmpty())
        {
            continue;
        }
        if (repair)
        {
            transaction->replaceEntity(entity.id, replacement);
            ++report.repaired_entity_count;
            has_repairs = true;
        }
        addIssue(report, VpAuditSeverity::Warning, entity.id, repairs.join(QStringLiteral("；")),
                 repair);
    }
    if (!layer(m_current_layer_name))
    {
        addIssue(report, VpAuditSeverity::Error, 0, tr("当前图层不存在。"), false);
    }
    if (repair && has_repairs)
    {
        transaction->commit();
    }
    return report;
}

} // namespace Vp
