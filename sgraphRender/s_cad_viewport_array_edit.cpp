#include "s_associative_array_geometry.h"
#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_document_transaction.h"

#include <QPainter>
#include <algorithm>
#include <cmath>
#include <utility>

namespace smartGraphics
{
namespace
{

const SAssociativeArrayData* selectedArrayData(const SCadDocument* document,
                                               const std::vector<SEntityId>& selected_ids)
{
    if (!document)
    {
        return nullptr;
    }
    for (SEntityId selected_id : selected_ids)
    {
        const auto iterator = std::find_if(document->entities().begin(), document->entities().end(),
                                           [selected_id](const SEntityRecord& entity)
                                           {
                                               return entity.id == selected_id;
                                           });
        if (iterator != document->entities().end() && iterator->associative_array)
        {
            return &*iterator->associative_array;
        }
    }
    return nullptr;
}

} // namespace

bool SCadViewport::editSelectedRectangularArray(int column_count, int row_count,
                                                double column_spacing, double row_spacing)
{
    const SAssociativeArrayData* selected_data =
        selectedArrayData(m_document, m_selected_entity_ids);
    if (!selected_data || selected_data->array_type != SArrayType::Rectangular ||
        column_count < 1 || row_count < 1 || (column_count == 1 && row_count == 1) ||
        static_cast<long long>(column_count) * row_count > 10000 ||
        !std::isfinite(column_spacing) || !std::isfinite(row_spacing) ||
        (column_count > 1 && std::abs(column_spacing) <= 1.0e-9) ||
        (row_count > 1 && std::abs(row_spacing) <= 1.0e-9))
    {
        emit commandMessage(tr("ARRAYEDIT RECT 需要已选择的矩形关联阵列和有效行列/间距。"));
        return false;
    }
    SAssociativeArrayData parameters = *selected_data;
    parameters.column_count = column_count;
    parameters.row_count = row_count;
    parameters.item_count = column_count * row_count;
    parameters.column_spacing = column_spacing;
    parameters.row_spacing = row_spacing;
    return editAssociativeArray(std::move(parameters));
}

bool SCadViewport::editSelectedPolarArray(int item_count, double fill_angle)
{
    const SAssociativeArrayData* selected_data =
        selectedArrayData(m_document, m_selected_entity_ids);
    if (!selected_data || selected_data->array_type != SArrayType::Polar || item_count < 2 ||
        item_count > 10000 || !std::isfinite(fill_angle) || std::abs(fill_angle) <= 1.0e-9 ||
        std::abs(fill_angle) > 360.0)
    {
        emit commandMessage(tr("ARRAYEDIT POLAR 需要已选择的环形关联阵列和有效项目数/填充角。"));
        return false;
    }
    SAssociativeArrayData parameters = *selected_data;
    parameters.item_count = item_count;
    parameters.fill_angle = fill_angle;
    return editAssociativeArray(std::move(parameters));
}

bool SCadViewport::editSelectedPathArray(int item_count, bool align_to_path)
{
    const SAssociativeArrayData* selected_data =
        selectedArrayData(m_document, m_selected_entity_ids);
    if (!selected_data || selected_data->array_type != SArrayType::Path || item_count < 2 ||
        item_count > 10000)
    {
        emit commandMessage(tr("ARRAYEDIT PATH 需要已选择的路径关联阵列和有效项目数。"));
        return false;
    }
    SAssociativeArrayData parameters = *selected_data;
    parameters.item_count = item_count;
    parameters.align_to_path = align_to_path;
    return editAssociativeArray(std::move(parameters));
}

bool SCadViewport::editAssociativeArray(SAssociativeArrayData parameters)
{
    std::vector<SEntityRecord> results;
    std::vector<SEntityId> existing_ids;
    if (!m_document ||
        !regenerateAssociativeArray(m_document->entities(), parameters, results, existing_ids) ||
        results.size() > 10000)
    {
        emit commandMessage(tr("ARRAYEDIT 无法重生成阵列；路径可能已删除或参数超出限制。"));
        return false;
    }
    auto transaction = m_document->beginTransaction(tr("编辑关联阵列"));
    std::vector<SEntityId> result_ids;
    result_ids.reserve(results.size());
    const std::size_t common_count = std::min(existing_ids.size(), results.size());
    for (std::size_t index = 0; index < common_count; ++index)
    {
        transaction->replaceEntity(existing_ids[index], std::move(results[index]));
        result_ids.push_back(existing_ids[index]);
    }
    for (std::size_t index = common_count; index < results.size(); ++index)
    {
        result_ids.push_back(transaction->addEntityCopy(std::move(results[index]), true));
    }
    for (std::size_t index = common_count; index < existing_ids.size(); ++index)
    {
        transaction->removeEntity(existing_ids[index]);
    }
    transaction->commit();
    m_selected_entity_ids = std::move(result_ids);
    m_selected_entity_id = m_selected_entity_ids.empty()
                               ? std::optional<SEntityId>()
                               : std::optional<SEntityId>(m_selected_entity_ids.front());
    emitSelectionState();
    emit commandMessage(tr("ARRAYEDIT 已重生成关联阵列，共 %1 个实体，可用 Undo 撤销。")
                            .arg(m_selected_entity_ids.size()));
    update();
    return true;
}

void SCadViewport::acceptArrayEditPoint(const SPoint2d& world_point)
{
    const std::optional<SEntityId> picked_id = entityAt(world_point);
    const SEntityRecord* picked = picked_id ? entityById(*picked_id) : nullptr;
    if (!picked || !picked->associative_array)
    {
        emit commandMessage(tr("ARRAYEDIT 请选择关联阵列中的任意实体："));
        return;
    }
    const SArrayId array_id = picked->associative_array->array_id;
    m_selected_entity_ids.clear();
    for (const SEntityRecord& entity : m_document->entities())
    {
        if (entity.associative_array && entity.associative_array->array_id == array_id)
        {
            m_selected_entity_ids.push_back(entity.id);
        }
    }
    m_selected_entity_id = picked->id;
    emitSelectionState();
    emit commandMessage(
        tr("ARRAYEDIT 已选择整组阵列。输入 RECT 列 行 列距 行距、POLAR 项目数 填充角，"
           "或 PATH 项目数 ALIGN|NOALIGN。"));
    update();
}

void SCadViewport::drawArrayEditPreview(QPainter& painter)
{
    const std::optional<SEntityId> hovered_id = entityAt(m_cursor_world);
    const SEntityRecord* hovered = hovered_id ? entityById(*hovered_id) : nullptr;
    if (!hovered || !hovered->associative_array)
    {
        return;
    }
    const SArrayId array_id = hovered->associative_array->array_id;
    painter.save();
    painter.setPen(QPen(QColor(92, 214, 255), 2.0, Qt::DashLine));
    for (const SEntityRecord& entity : m_document->entities())
    {
        if (entity.associative_array && entity.associative_array->array_id == array_id)
        {
            drawEntityGeometry(painter, entity, QColor(92, 214, 255, 72));
        }
    }
    painter.restore();
}

} // namespace smartGraphics
