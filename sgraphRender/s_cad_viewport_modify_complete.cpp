#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_viewport_geometry.h"
#include "s_document_transaction.h"
#include "s_entity.h"

#include <utility>

namespace vectorPath
{

void SCadViewport::completeMove(const SPoint2d& destination)
{
    if (!m_selected_entity_ids.empty() && m_first_point)
    {
        const double delta_x = destination.x - m_first_point->x;
        const double delta_y = destination.y - m_first_point->y;
        auto transaction = m_document->beginTransaction(tr("移动实体"));
        for (SEntityId entity_id : m_selected_entity_ids)
        {
            if (const SEntityRecord* source = entityById(entity_id))
            {
                transaction->replaceEntity(source->id, translatedEntity(*source, delta_x, delta_y));
            }
        }
        transaction->commit();
    }
    m_first_point.reset();
    emit commandMessage(tr("MOVE 指定基点："));
    update();
}

void SCadViewport::completeCopy(const SPoint2d& destination)
{
    std::vector<SEntityId> copied_ids;
    if (!m_selected_entity_ids.empty() && m_first_point)
    {
        const double delta_x = destination.x - m_first_point->x;
        const double delta_y = destination.y - m_first_point->y;
        auto transaction = m_document->beginTransaction(tr("复制实体"));
        for (SEntityId entity_id : m_selected_entity_ids)
        {
            if (const SEntityRecord* source = entityById(entity_id))
            {
                copied_ids.push_back(
                    transaction->addEntityCopy(translatedEntity(*source, delta_x, delta_y)));
            }
        }
        transaction->commit();
    }
    if (!copied_ids.empty())
    {
        m_selected_entity_ids = std::move(copied_ids);
        m_selected_entity_id = m_selected_entity_ids.front();
        emitSelectionState();
    }
    m_first_point.reset();
    emit commandMessage(tr("COPY 指定基点："));
    update();
}

void SCadViewport::completeRotate(const SPoint2d& destination)
{
    if (!m_selected_entity_ids.empty() && m_first_point)
    {
        const double angle = entityAngleDegrees(*m_first_point, destination);
        auto transaction = m_document->beginTransaction(tr("旋转实体"));
        for (SEntityId entity_id : m_selected_entity_ids)
        {
            if (const SEntityRecord* source = entityById(entity_id))
            {
                SEntityRecord replacement = rotatedEntity(*source, *m_first_point, angle);
                replacement.associative_array.reset();
                transaction->replaceEntity(source->id, std::move(replacement));
            }
        }
        transaction->commit();
    }
    m_first_point.reset();
    emit commandMessage(tr("ROTATE 指定基点："));
    update();
}

void SCadViewport::completeScale(const SPoint2d& destination)
{
    if (!m_selected_entity_ids.empty() && m_first_point && !m_input_points.empty())
    {
        const double reference_length = distance(*m_first_point, m_input_points.front());
        const double target_length = distance(*m_first_point, destination);
        if (reference_length > 1.0e-9 && target_length > 1.0e-9)
        {
            const double scale_factor = target_length / reference_length;
            auto transaction = m_document->beginTransaction(tr("缩放实体"));
            for (SEntityId entity_id : m_selected_entity_ids)
            {
                if (const SEntityRecord* source = entityById(entity_id))
                {
                    SEntityRecord replacement = scaledEntity(*source, *m_first_point, scale_factor);
                    replacement.associative_array.reset();
                    transaction->replaceEntity(source->id, std::move(replacement));
                }
            }
            transaction->commit();
        }
    }
    m_first_point.reset();
    m_input_points.clear();
    emit commandMessage(tr("SCALE 指定基点："));
    update();
}

void SCadViewport::completeMirror(const SPoint2d& axis_end)
{
    std::vector<SEntityId> mirrored_ids;
    if (!m_selected_entity_ids.empty() && m_first_point &&
        distance(*m_first_point, axis_end) > 1.0e-9)
    {
        auto transaction = m_document->beginTransaction(tr("镜像实体"));
        for (SEntityId entity_id : m_selected_entity_ids)
        {
            if (const SEntityRecord* source = entityById(entity_id))
            {
                mirrored_ids.push_back(
                    transaction->addEntityCopy(mirroredEntity(*source, *m_first_point, axis_end)));
            }
        }
        transaction->commit();
    }
    if (!mirrored_ids.empty())
    {
        m_selected_entity_ids = std::move(mirrored_ids);
        m_selected_entity_id = m_selected_entity_ids.front();
        emitSelectionState();
    }
    m_first_point.reset();
    emit commandMessage(tr("MIRROR 指定镜像线第一点："));
    update();
}

void SCadViewport::completeOffset(const SPoint2d& through_point)
{
    if (!m_selected_entity_id)
    {
        return;
    }
    const SEntityRecord* source = entityById(*m_selected_entity_id);
    if (!source)
    {
        return;
    }
    SEntityRecord offset_entity;
    if (!offsetEntity(*source, through_point, offset_entity))
    {
        emit commandMessage(tr("无法在指定一侧生成有效偏移，请更换通过点。"));
        return;
    }
    auto transaction = m_document->beginTransaction(tr("偏移实体"));
    transaction->addEntityCopy(std::move(offset_entity));
    transaction->commit();
    emit commandMessage(source->type == SEntityType::Spline
                            ? tr("OFFSET 已将样条的偏移结果转换为高精度多段线。指定通过点：")
                            : tr("OFFSET 指定通过点："));
    update();
}

void SCadViewport::completeJoin()
{
    if (!m_document || m_selected_entity_ids.size() < 2)
    {
        emit commandMessage(tr("JOIN 至少需要两个相连的同类实体。"));
        return;
    }
    std::vector<SEntityRecord> sources;
    sources.reserve(m_selected_entity_ids.size());
    for (SEntityId entity_id : m_selected_entity_ids)
    {
        const SEntityRecord* source = entityById(entity_id);
        if (!source || (source->type != SEntityType::Line && source->type != SEntityType::Arc &&
                        source->type != SEntityType::Polyline))
        {
            emit commandMessage(tr("JOIN 当前选择集中包含不支持的实体。"));
            return;
        }
        sources.push_back(*source);
    }
    SEntityRecord joined_entity;
    if (!joinedEntity(sources, 1.0e-6, joined_entity))
    {
        emit commandMessage(tr("JOIN 所选实体不是连续直线/多段线链或同圆连续圆弧。"));
        return;
    }

    const SEntityId result_id = sources.front().id;
    auto transaction = m_document->beginTransaction(tr("合并实体"));
    transaction->replaceEntity(result_id, std::move(joined_entity));
    for (std::size_t index = 1; index < sources.size(); ++index)
    {
        transaction->removeEntity(sources[index].id);
    }
    transaction->commit();
    m_selected_entity_ids = {result_id};
    m_selected_entity_id = result_id;
    emitSelectionState();
    emit commandMessage(tr("JOIN 已合并 %1 个实体。继续选择或按 Esc 结束。").arg(sources.size()));
    update();
}

void SCadViewport::completePolyline(bool is_closed)
{
    const std::size_t minimum_point_count = is_closed ? 3U : 2U;
    if (m_document && m_input_points.size() >= minimum_point_count)
    {
        m_polyline_bulges.resize(m_input_points.size(), 0.0);
        m_polyline_start_widths.resize(m_input_points.size(), 0.0);
        m_polyline_end_widths.resize(m_input_points.size(), 0.0);
        if (is_closed && !m_input_points.empty())
        {
            m_polyline_start_widths.back() = m_polyline_start_width;
            m_polyline_end_widths.back() = m_polyline_end_width;
        }
        auto transaction = m_document->beginTransaction(tr("创建多段线"));
        transaction->addPolyline(m_input_points, is_closed, m_polyline_bulges,
                                 m_polyline_start_widths, m_polyline_end_widths);
        transaction->commit();
    }
    m_input_points.clear();
    m_polyline_bulges.clear();
    m_polyline_start_widths.clear();
    m_polyline_end_widths.clear();
    m_polyline_arc_point.reset();
    emit commandMessage(is_closed ? tr("PLINE 已闭合。指定新起点：")
                                  : tr("PLINE 已完成。指定新起点："));
    update();
}

} // namespace vectorPath
