#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_viewport_geometry.h"
#include "s_document_transaction.h"

#include <QPainter>

namespace smartCam
{

SEntityRecord alignedEntity(const SEntityRecord& source, const SPoint2d& first_source_point,
                            const SPoint2d& first_target_point, const SPoint2d& second_source_point,
                            const SPoint2d& second_target_point, bool scale_to_fit)
{
    SEntityRecord result = translatedEntity(source, first_target_point.x - first_source_point.x,
                                            first_target_point.y - first_source_point.y);
    const double source_length = distance(first_source_point, second_source_point);
    const double target_length = distance(first_target_point, second_target_point);
    if (source_length <= 1.0e-9 || target_length <= 1.0e-9)
    {
        return result;
    }
    const double source_angle = entityAngleDegrees(first_source_point, second_source_point);
    const double target_angle = entityAngleDegrees(first_target_point, second_target_point);
    result = rotatedEntity(result, first_target_point, target_angle - source_angle);
    if (scale_to_fit)
    {
        result = scaledEntity(result, first_target_point, target_length / source_length);
    }
    return result;
}

void SCadViewport::acceptAlignPoint(const SPoint2d& world_point)
{
    if (m_selected_entity_ids.empty())
    {
        selectAt(world_point);
        if (!m_selected_entity_ids.empty())
        {
            emit commandMessage(
                tr("ALIGN 已选择 %1 个实体，指定第一个源点：").arg(m_selected_entity_ids.size()));
        }
        return;
    }
    m_input_points.push_back(world_point);
    if (m_input_points.size() == 1)
    {
        emit commandMessage(tr("ALIGN 指定第一个目标点："));
        update();
        return;
    }
    if (m_input_points.size() == 2)
    {
        emit commandMessage(tr("ALIGN 指定第二个源点："));
        update();
        return;
    }
    if (m_input_points.size() == 3)
    {
        emit commandMessage(tr("ALIGN 指定第二个目标点（将等比缩放）："));
        update();
        return;
    }
    if (m_input_points.size() != 4 || distance(m_input_points[0], m_input_points[2]) <= 1.0e-9 ||
        distance(m_input_points[1], m_input_points[3]) <= 1.0e-9)
    {
        m_input_points.clear();
        emit commandMessage(tr("ALIGN 点对无效，请重新指定第一个源点："));
        update();
        return;
    }
    auto transaction = m_document->beginTransaction(tr("对齐实体"));
    for (SEntityId entity_id : m_selected_entity_ids)
    {
        if (const SEntityRecord* source = entityById(entity_id))
        {
            transaction->replaceEntity(entity_id,
                                       alignedEntity(*source, m_input_points[0], m_input_points[1],
                                                     m_input_points[2], m_input_points[3],
                                                     m_align_scale_enabled));
        }
    }
    transaction->commit();
    m_input_points.clear();
    emit commandMessage(tr("ALIGN 已完成平移、旋转和等比缩放。指定新的第一个源点："));
    update();
}

void SCadViewport::drawAlignPreview(QPainter& painter)
{
    if (m_selected_entity_ids.empty() || m_input_points.empty())
    {
        return;
    }
    painter.setPen(QPen(QColor(255, 194, 92), 1.2, Qt::DashLine));
    if (m_input_points.size() >= 2)
    {
        painter.drawLine(worldToScreen(m_input_points[0]), worldToScreen(m_input_points[1]));
    }
    if (m_input_points.size() >= 3)
    {
        painter.drawLine(worldToScreen(m_input_points[2]), worldToScreen(m_cursor_world));
    }
    if (m_input_points.size() < 2)
    {
        return;
    }
    painter.setPen(QPen(QColor(92, 214, 255), 1.7, Qt::DashLine));
    for (SEntityId entity_id : m_selected_entity_ids)
    {
        if (const SEntityRecord* source = entityById(entity_id))
        {
            const SEntityRecord preview =
                m_input_points.size() < 3
                    ? translatedEntity(*source, m_input_points[1].x - m_input_points[0].x,
                                       m_input_points[1].y - m_input_points[0].y)
                    : alignedEntity(*source, m_input_points[0], m_input_points[1],
                                    m_input_points[2], m_cursor_world, m_align_scale_enabled);
            drawEntityGeometry(painter, preview, QColor(92, 214, 255, 72));
        }
    }
}

} // namespace smartCam
