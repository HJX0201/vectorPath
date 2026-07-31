#include "s_associative_array_geometry.h"
#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_viewport_geometry.h"
#include "s_document_transaction.h"

#include <QPainter>
#include <algorithm>
#include <cmath>
#include <utility>

namespace vectorPath
{

std::vector<SEntityRecord> rectangularArrayEntities(const std::vector<SEntityRecord>& sources,
                                                    double column_spacing, double row_spacing,
                                                    int column_count, int row_count)
{
    std::vector<SEntityRecord> results;
    const std::size_t entity_count = column_count > 0 && row_count > 0
                                         ? sources.size() * static_cast<std::size_t>(column_count) *
                                               static_cast<std::size_t>(row_count)
                                         : 0U;
    if (sources.empty() || column_count < 1 || row_count < 1 || entity_count > 10000U ||
        (column_count == 1 && row_count == 1) ||
        static_cast<long long>(column_count) * row_count > 10000)
    {
        return results;
    }
    results.reserve(entity_count - sources.size());
    for (int row = 0; row < row_count; ++row)
    {
        for (int column = 0; column < column_count; ++column)
        {
            if (row == 0 && column == 0)
            {
                continue;
            }
            for (const SEntityRecord& source : sources)
            {
                results.push_back(
                    translatedEntity(source, column * column_spacing, row * row_spacing));
            }
        }
    }
    return results;
}

void SCadViewport::setRectangularArrayCounts(int column_count, int row_count)
{
    if (column_count < 1 || row_count < 1 || (column_count == 1 && row_count == 1) ||
        static_cast<long long>(column_count) * row_count > 10000)
    {
        emit commandMessage(tr("ARRAYRECT 行列数无效，总项目数必须介于 2 和 10000。"));
        return;
    }
    m_array_column_count = column_count;
    m_array_row_count = row_count;
    emit commandMessage(tr("ARRAYRECT 已设为 %1 列 × %2 行。选择对象或指定基点：")
                            .arg(column_count)
                            .arg(row_count));
}

void SCadViewport::acceptRectangularArrayPoint(const SPoint2d& world_point)
{
    if (m_selected_entity_ids.empty())
    {
        selectAt(world_point);
        emit commandMessage(m_selected_entity_ids.empty() ? tr("ARRAYRECT 未选择对象，请重试：")
                                                          : tr("ARRAYRECT 指定基点："));
        return;
    }
    if (!m_first_point)
    {
        m_first_point = world_point;
        emit commandMessage(tr("ARRAYRECT 指定列间距和行间距点："));
        update();
        return;
    }
    const double column_spacing = world_point.x - m_first_point->x;
    const double row_spacing = world_point.y - m_first_point->y;
    if ((m_array_column_count > 1 && std::abs(column_spacing) <= 1.0e-9) ||
        (m_array_row_count > 1 && std::abs(row_spacing) <= 1.0e-9))
    {
        emit commandMessage(tr("ARRAYRECT 所需方向的间距不能为零，请重新指定："));
        return;
    }
    std::vector<SEntityRecord> sources;
    for (SEntityId entity_id : m_selected_entity_ids)
    {
        if (const SEntityRecord* source = entityById(entity_id))
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
        emit commandMessage(tr("ARRAYRECT 不能嵌套关联阵列，请先使用 ARRAYEDIT 或分解。"));
        return;
    }
    std::vector<SEntityRecord> array =
        associativeRectangularArrayEntities(sources, sources.front().id, column_spacing,
                                            row_spacing, m_array_column_count, m_array_row_count);
    if (array.empty())
    {
        emit commandMessage(tr("ARRAYRECT 阵列为空或项目总数超过限制。"));
        return;
    }
    auto transaction = m_document->beginTransaction(tr("矩形阵列"));
    std::vector<SEntityId> result_ids;
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
    m_selected_entity_id = m_selected_entity_ids.empty()
                               ? std::optional<SEntityId>()
                               : std::optional<SEntityId>(m_selected_entity_ids.front());
    m_first_point.reset();
    emitSelectionState();
    emit commandMessage(
        tr("ARRAYRECT 已创建包含 %1 个实体的关联阵列。").arg(m_selected_entity_ids.size()));
    update();
}

void SCadViewport::drawRectangularArrayPreview(QPainter& painter)
{
    if (!m_first_point || m_selected_entity_ids.empty())
    {
        return;
    }
    const double column_spacing = m_cursor_world.x - m_first_point->x;
    const double row_spacing = m_cursor_world.y - m_first_point->y;
    std::vector<SEntityRecord> sources;
    for (SEntityId entity_id : m_selected_entity_ids)
    {
        if (const SEntityRecord* source = entityById(entity_id))
        {
            sources.push_back(*source);
        }
    }
    const std::vector<SEntityRecord> previews = rectangularArrayEntities(
        sources, column_spacing, row_spacing, m_array_column_count, m_array_row_count);
    painter.setPen(QPen(QColor(255, 194, 92), 1.2, Qt::DashLine));
    painter.drawLine(worldToScreen(*m_first_point), worldToScreen(m_cursor_world));
    painter.setPen(QPen(QColor(92, 214, 255), 1.4, Qt::DashLine));
    for (const SEntityRecord& preview : previews)
    {
        drawEntityGeometry(painter, preview, QColor(92, 214, 255, 60));
    }
}

} // namespace vectorPath
