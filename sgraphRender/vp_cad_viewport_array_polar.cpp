#include "vp_associative_array_geometry.h"
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

std::vector<VpEntityRecord> polarArrayEntities(const std::vector<VpEntityRecord>& sources,
                                               const VpPoint2d& center, int item_count,
                                               double fill_angle)
{
    std::vector<VpEntityRecord> results;
    const std::size_t entity_count =
        item_count > 1 ? sources.size() * static_cast<std::size_t>(item_count) : 0;
    const std::size_t copy_count =
        entity_count > sources.size() ? entity_count - sources.size() : 0;
    if (sources.empty() || item_count < 2 || std::abs(fill_angle) <= 1.0e-9 ||
        std::abs(fill_angle) > 360.0 + 1.0e-9 || entity_count > 10000)
    {
        return results;
    }
    const bool is_full_circle = std::abs(std::abs(fill_angle) - 360.0) <= 1.0e-9;
    const double angle_step =
        fill_angle / static_cast<double>(is_full_circle ? item_count : item_count - 1);
    results.reserve(copy_count);
    for (int item = 1; item < item_count; ++item)
    {
        for (const VpEntityRecord& source : sources)
        {
            results.push_back(rotatedEntity(source, center, item * angle_step));
        }
    }
    return results;
}

void VpCadViewport::setPolarArrayParameters(int item_count, double fill_angle)
{
    if (item_count < 2 || item_count > 10000 || std::abs(fill_angle) <= 1.0e-9 ||
        std::abs(fill_angle) > 360.0 || !std::isfinite(fill_angle))
    {
        emit commandMessage(tr("ARRAYPOLAR 项目数必须为 2–10000，填充角必须在 -360° 到 360°。"));
        return;
    }
    m_array_polar_item_count = item_count;
    m_array_polar_fill_angle = fill_angle;
    emit commandMessage(tr("ARRAYPOLAR 已设为 %1 项，填充角 %2°。选择对象或指定中心点：")
                            .arg(item_count)
                            .arg(fill_angle, 0, 'f', 1));
}

void VpCadViewport::acceptPolarArrayPoint(const VpPoint2d& world_point)
{
    if (m_selected_entity_ids.empty())
    {
        selectAt(world_point);
        emit commandMessage(m_selected_entity_ids.empty() ? tr("ARRAYPOLAR 未选择对象，请重试：")
                                                          : tr("ARRAYPOLAR 指定中心点："));
        return;
    }
    if (!m_first_point)
    {
        m_first_point = world_point;
        emit commandMessage(tr("ARRAYPOLAR 指定参考方向点以确认阵列："));
        update();
        return;
    }
    if (distance(*m_first_point, world_point) <= 1.0e-9)
    {
        emit commandMessage(tr("ARRAYPOLAR 参考方向点不能与中心点重合："));
        return;
    }
    std::vector<VpEntityRecord> sources;
    for (VpEntityId entity_id : m_selected_entity_ids)
    {
        if (const VpEntityRecord* source = entityById(entity_id))
        {
            sources.push_back(*source);
        }
    }
    if (sources.empty() || std::any_of(sources.begin(), sources.end(),
                                       [](const auto& source)
                                       {
                                           return source.associative_array.has_value();
                                       }))
    {
        emit commandMessage(tr("ARRAYPOLAR 不能嵌套关联阵列，请先使用 ARRAYEDIT 或分解。"));
        return;
    }
    std::vector<VpEntityRecord> array =
        associativePolarArrayEntities(sources, sources.front().id, *m_first_point,
                                      m_array_polar_item_count, m_array_polar_fill_angle);
    if (array.empty())
    {
        emit commandMessage(tr("ARRAYPOLAR 阵列为空或副本总数超过 10000。"));
        return;
    }
    auto transaction = m_document->beginTransaction(tr("环形阵列"));
    std::vector<VpEntityId> result_ids;
    result_ids.reserve(array.size());
    for (std::size_t index = 0; index < sources.size(); ++index)
    {
        transaction->replaceEntity(sources[index].id, std::move(array[index]));
        result_ids.push_back(sources[index].id);
    }
    for (std::size_t index = sources.size(); index < array.size(); ++index)
    {
        result_ids.push_back(transaction->addEntityCopy(std::move(array[index]), true));
    }
    transaction->commit();
    m_selected_entity_ids = std::move(result_ids);
    m_selected_entity_id = m_selected_entity_ids.front();
    m_first_point.reset();
    emitSelectionState();
    emit commandMessage(
        tr("ARRAYPOLAR 已创建包含 %1 个实体的关联阵列。").arg(m_selected_entity_ids.size()));
    update();
}

void VpCadViewport::drawPolarArrayPreview(QPainter& painter)
{
    if (!m_first_point || m_selected_entity_ids.empty())
    {
        return;
    }
    std::vector<VpEntityRecord> sources;
    for (VpEntityId entity_id : m_selected_entity_ids)
    {
        if (const VpEntityRecord* source = entityById(entity_id))
        {
            sources.push_back(*source);
        }
    }
    const std::vector<VpEntityRecord> previews = polarArrayEntities(
        sources, *m_first_point, m_array_polar_item_count, m_array_polar_fill_angle);
    painter.setPen(QPen(QColor(255, 194, 92), 1.2, Qt::DashLine));
    painter.drawLine(worldToScreen(*m_first_point), worldToScreen(m_cursor_world));
    painter.setBrush(QColor(255, 194, 92));
    painter.drawEllipse(worldToScreen(*m_first_point), 3.5, 3.5);
    painter.setPen(QPen(QColor(92, 214, 255), 1.4, Qt::DashLine));
    for (const VpEntityRecord& preview : previews)
    {
        drawEntityGeometry(painter, preview, QColor(92, 214, 255, 60));
    }
}

} // namespace Vp
