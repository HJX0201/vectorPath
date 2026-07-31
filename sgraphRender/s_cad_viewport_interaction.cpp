#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_viewport_geometry.h"
#include "s_document_transaction.h"
#include "s_entity.h"

#include <QKeyEvent>
#include <QKeySequence>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace vectorPath
{

void SCadViewport::acceptPoint(const SPoint2d& world_point)
{
    if (!m_document)
    {
        return;
    }
    if (m_tool_mode == SToolMode::Select)
    {
        selectAt(world_point);
        return;
    }
    if (m_tool_mode == SToolMode::Erase)
    {
        selectAt(world_point);
        if (!m_selected_entity_ids.empty())
        {
            deleteSelected();
            emit commandMessage(tr("ERASE 选择要删除的对象："));
        }
        update();
        return;
    }
    if (m_tool_mode == SToolMode::PolylineEdit)
    {
        acceptPolylineEditPoint(world_point);
        return;
    }
    if (m_tool_mode == SToolMode::SplineEdit)
    {
        acceptSplineEditPoint(world_point);
        return;
    }
    if (m_tool_mode == SToolMode::MText)
    {
        acceptMTextPoint(world_point);
        return;
    }
    if (m_tool_mode == SToolMode::Leader)
    {
        acceptLeaderPoint(world_point);
        return;
    }
    if (m_tool_mode == SToolMode::Lengthen)
    {
        acceptLengthenPoint(world_point);
        return;
    }
    if (m_tool_mode == SToolMode::Trim)
    {
        const std::optional<SEntityId> picked_id = entityAt(world_point);
        const SEntityRecord* picked_entity = picked_id ? entityById(*picked_id) : nullptr;
        if (!m_reference_entity_id)
        {
            if (picked_entity && (picked_entity->type == SEntityType::Line ||
                                  picked_entity->type == SEntityType::Circle ||
                                  picked_entity->type == SEntityType::Arc))
            {
                m_reference_entity_id = picked_entity->id;
                m_selected_entity_id = picked_entity->id;
                m_selected_entity_ids = {picked_entity->id};
                emitSelectionState();
                emit commandMessage(tr("TRIM 选择要修剪的直线段："));
            }
            else
            {
                emit commandMessage(tr("TRIM 剪切边必须是直线、圆或圆弧。"));
            }
            update();
            return;
        }
        const SEntityRecord* cutting_entity = entityById(*m_reference_entity_id);
        if (!cutting_entity || !picked_entity ||
            (picked_entity->type != SEntityType::Line &&
             picked_entity->type != SEntityType::Circle &&
             picked_entity->type != SEntityType::Arc &&
             picked_entity->type != SEntityType::Polyline) ||
            picked_entity->id == cutting_entity->id)
        {
            emit commandMessage(tr("TRIM 请选择与剪切边相交的直线、圆、圆弧或多段线。"));
            return;
        }
        std::vector<SEntityRecord> replacements =
            trimmedEntityParts(*cutting_entity, *picked_entity, world_point);
        if (replacements.empty())
        {
            emit commandMessage(tr("TRIM 两个实体在有限边界范围内没有可用交点。"));
            return;
        }
        auto transaction = m_document->beginTransaction(tr("修剪实体"));
        transaction->replaceEntity(picked_entity->id, std::move(replacements.front()));
        for (std::size_t index = 1; index < replacements.size(); ++index)
        {
            transaction->addEntityCopy(std::move(replacements[index]));
        }
        transaction->commit();
        emit commandMessage(
            tr("TRIM 已生成 %1 段结果。继续选择目标，Esc 结束：").arg(replacements.size()));
        update();
        return;
    }
    if (m_tool_mode == SToolMode::Extend)
    {
        const std::optional<SEntityId> picked_id = entityAt(world_point);
        const SEntityRecord* picked_entity = picked_id ? entityById(*picked_id) : nullptr;
        if (!m_reference_entity_id)
        {
            if (picked_entity && (picked_entity->type == SEntityType::Line ||
                                  picked_entity->type == SEntityType::Circle ||
                                  picked_entity->type == SEntityType::Arc))
            {
                m_reference_entity_id = picked_entity->id;
                m_selected_entity_id = picked_entity->id;
                m_selected_entity_ids = {picked_entity->id};
                emitSelectionState();
                emit commandMessage(tr("EXTEND 选择要延伸的直线或圆弧："));
            }
            else
            {
                emit commandMessage(tr("EXTEND 边界必须是直线、圆或圆弧。"));
            }
            update();
            return;
        }
        const SEntityRecord* boundary_entity = entityById(*m_reference_entity_id);
        if (!boundary_entity || !picked_entity ||
            (picked_entity->type != SEntityType::Line && picked_entity->type != SEntityType::Arc &&
             picked_entity->type != SEntityType::Polyline) ||
            picked_entity->id == boundary_entity->id)
        {
            emit commandMessage(tr("EXTEND 请选择可延伸到边界的直线、圆弧或开放多段线。"));
            return;
        }
        SEntityRecord replacement;
        if (!extendedEntity(*boundary_entity, *picked_entity, world_point, replacement))
        {
            emit commandMessage(tr("EXTEND 目标直线无法延伸到所选边界。"));
            return;
        }
        auto transaction = m_document->beginTransaction(tr("延伸实体"));
        transaction->replaceEntity(picked_entity->id, std::move(replacement));
        transaction->commit();
        emit commandMessage(tr("EXTEND 继续选择要延伸的直线、圆弧或开放多段线，Esc 结束："));
        update();
        return;
    }
    if (m_tool_mode == SToolMode::Break)
    {
        if (!m_reference_entity_id)
        {
            const std::optional<SEntityId> picked_id = entityAt(world_point);
            const SEntityRecord* picked_entity = picked_id ? entityById(*picked_id) : nullptr;
            if (!picked_entity || (picked_entity->type != SEntityType::Line &&
                                   picked_entity->type != SEntityType::Circle &&
                                   picked_entity->type != SEntityType::Arc))
            {
                emit commandMessage(tr("BREAK 请选择直线、圆或圆弧。"));
                return;
            }
            m_reference_entity_id = picked_entity->id;
            m_first_point = world_point;
            m_selected_entity_id = picked_entity->id;
            m_selected_entity_ids = {picked_entity->id};
            emitSelectionState();
            emit commandMessage(tr("BREAK 指定第二个打断点；同一点可拆分直线："));
            update();
            return;
        }
        const SEntityRecord* source = entityById(*m_reference_entity_id);
        if (!source || !m_first_point)
        {
            cancelCommand();
            return;
        }
        std::vector<SEntityRecord> pieces = brokenEntityParts(*source, *m_first_point, world_point);
        if (pieces.empty())
        {
            emit commandMessage(tr("BREAK 打断点没有产生有效实体段。"));
            return;
        }
        auto transaction = m_document->beginTransaction(tr("打断实体"));
        std::vector<SEntityId> piece_ids{source->id};
        transaction->replaceEntity(source->id, pieces.front());
        for (std::size_t index = 1; index < pieces.size(); ++index)
        {
            piece_ids.push_back(transaction->addEntityCopy(std::move(pieces[index])));
        }
        transaction->commit();
        m_selected_entity_ids = std::move(piece_ids);
        m_selected_entity_id = m_selected_entity_ids.front();
        m_reference_entity_id.reset();
        m_first_point.reset();
        emitSelectionState();
        emit commandMessage(tr("BREAK 选择下一条直线、圆或圆弧和第一个打断点："));
        update();
        return;
    }
    if (m_tool_mode == SToolMode::Join)
    {
        const std::optional<SEntityId> picked_id = entityAt(world_point);
        const SEntityRecord* picked_entity = picked_id ? entityById(*picked_id) : nullptr;
        if (!picked_entity ||
            (picked_entity->type != SEntityType::Line && picked_entity->type != SEntityType::Arc &&
             picked_entity->type != SEntityType::Polyline))
        {
            emit commandMessage(tr("JOIN 请选择直线、开放多段线或圆弧。"));
            return;
        }
        if (!m_selected_entity_ids.empty())
        {
            const SEntityRecord* first_source = entityById(m_selected_entity_ids.front());
            const bool first_is_linear =
                first_source && (first_source->type == SEntityType::Line ||
                                 first_source->type == SEntityType::Polyline);
            const bool picked_is_linear = picked_entity->type == SEntityType::Line ||
                                          picked_entity->type == SEntityType::Polyline;
            if (first_source && first_source->type != picked_entity->type &&
                !(first_is_linear && picked_is_linear))
            {
                emit commandMessage(tr("JOIN 线性实体和圆弧不能在同一选择集中合并。"));
                return;
            }
        }
        const auto iterator = std::find(m_selected_entity_ids.begin(), m_selected_entity_ids.end(),
                                        picked_entity->id);
        if (iterator == m_selected_entity_ids.end())
        {
            m_selected_entity_ids.push_back(picked_entity->id);
        }
        else
        {
            m_selected_entity_ids.erase(iterator);
        }
        m_selected_entity_id = m_selected_entity_ids.empty()
                                   ? std::optional<SEntityId>()
                                   : std::optional<SEntityId>(m_selected_entity_ids.front());
        emitSelectionState();
        emit commandMessage(
            tr("JOIN 已选择 %1 个同类实体，Enter 完成：").arg(m_selected_entity_ids.size()));
        update();
        return;
    }
    if (m_tool_mode == SToolMode::Explode)
    {
        const std::optional<SEntityId> picked_id = entityAt(world_point);
        const SEntityRecord* source = picked_id ? entityById(*picked_id) : nullptr;
        if (!source)
        {
            emit commandMessage(tr("EXPLODE 未找到可分解实体。"));
            return;
        }
        std::vector<SEntityRecord> parts = explodedEntityParts(*source);
        if (parts.empty())
        {
            emit commandMessage(tr("EXPLODE 当前实体无法分解。"));
            return;
        }
        auto transaction = m_document->beginTransaction(tr("分解实体"));
        std::vector<SEntityId> part_ids{source->id};
        transaction->replaceEntity(source->id, parts.front());
        for (std::size_t index = 1; index < parts.size(); ++index)
        {
            part_ids.push_back(transaction->addEntityCopy(std::move(parts[index])));
        }
        transaction->commit();
        m_selected_entity_ids = std::move(part_ids);
        m_selected_entity_id = m_selected_entity_ids.front();
        emitSelectionState();
        emit commandMessage(tr("EXPLODE 已分解为 %1 个基础实体。继续选择或按 Esc 结束。")
                                .arg(m_selected_entity_ids.size()));
        update();
        return;
    }
    if (m_tool_mode == SToolMode::Stretch)
    {
        if (m_input_points.size() < 2)
        {
            m_input_points.push_back(world_point);
            if (m_input_points.size() == 1)
            {
                emit commandMessage(tr("STRETCH 指定交叉窗口另一个角点："));
                update();
                return;
            }
            selectInWindow(m_input_points[0], m_input_points[1], true);
            m_selected_entity_ids.erase(
                std::remove_if(m_selected_entity_ids.begin(), m_selected_entity_ids.end(),
                               [this](SEntityId entity_id)
                               {
                                   const SEntityRecord* source = entityById(entity_id);
                                   SEntityRecord preview;
                                   return !source ||
                                          !stretchedEntity(*source, m_input_points[0],
                                                           m_input_points[1], 0.0, 0.0, preview);
                               }),
                m_selected_entity_ids.end());
            m_selected_entity_id = m_selected_entity_ids.empty()
                                       ? std::optional<SEntityId>()
                                       : std::optional<SEntityId>(m_selected_entity_ids.front());
            emitSelectionState();
            if (m_selected_entity_ids.empty())
            {
                m_input_points.clear();
                emit commandMessage(tr("STRETCH 窗口内没有可拉伸控制点，请重新指定："));
            }
            else
            {
                emit commandMessage(
                    tr("STRETCH 已选择 %1 个实体，指定基点：").arg(m_selected_entity_ids.size()));
            }
            update();
            return;
        }
        if (!m_first_point)
        {
            m_first_point = world_point;
            emit commandMessage(tr("STRETCH 指定位移第二点："));
            update();
            return;
        }
        completeStretch(world_point);
        return;
    }
    if (m_tool_mode == SToolMode::Fillet)
    {
        acceptFilletPoint(world_point);
        return;
    }
    if (m_tool_mode == SToolMode::Chamfer)
    {
        acceptChamferPoint(world_point);
        return;
    }
    if (m_tool_mode == SToolMode::Blend)
    {
        acceptBlendPoint(world_point);
        return;
    }
    if (m_tool_mode == SToolMode::Align)
    {
        acceptAlignPoint(world_point);
        return;
    }
    if (m_tool_mode == SToolMode::ArrayRect)
    {
        acceptRectangularArrayPoint(world_point);
        return;
    }
    if (m_tool_mode == SToolMode::ArrayPolar)
    {
        acceptPolarArrayPoint(world_point);
        return;
    }
    if (m_tool_mode == SToolMode::ArrayPath)
    {
        acceptPathArrayPoint(world_point);
        return;
    }
    if (m_tool_mode == SToolMode::ArrayEdit)
    {
        acceptArrayEditPoint(world_point);
        return;
    }
    if (m_tool_mode == SToolMode::Move || m_tool_mode == SToolMode::Copy ||
        m_tool_mode == SToolMode::Rotate || m_tool_mode == SToolMode::Scale ||
        m_tool_mode == SToolMode::Mirror || m_tool_mode == SToolMode::Offset)
    {
        if (!m_selected_entity_id)
        {
            selectAt(world_point);
            if (m_selected_entity_id)
            {
                if (m_tool_mode == SToolMode::Offset)
                {
                    emit commandMessage(tr("OFFSET 指定通过点："));
                }
                else if (m_tool_mode == SToolMode::Mirror)
                {
                    emit commandMessage(tr("MIRROR 指定镜像线第一点："));
                }
                else if (m_tool_mode == SToolMode::Scale)
                {
                    emit commandMessage(tr("SCALE 指定基点："));
                }
                else if (m_tool_mode == SToolMode::Copy)
                {
                    emit commandMessage(tr("COPY 指定基点："));
                }
                else
                {
                    emit commandMessage(m_tool_mode == SToolMode::Move ? tr("MOVE 指定基点：")
                                                                       : tr("ROTATE 指定基点："));
                }
            }
            return;
        }
        if (m_tool_mode == SToolMode::Offset)
        {
            completeOffset(world_point);
            return;
        }
        if (!m_first_point)
        {
            m_first_point = world_point;
            if (m_tool_mode == SToolMode::Mirror)
            {
                emit commandMessage(tr("MIRROR 指定镜像线第二点："));
            }
            else if (m_tool_mode == SToolMode::Scale)
            {
                emit commandMessage(tr("SCALE 指定参考长度点："));
            }
            else if (m_tool_mode == SToolMode::Copy)
            {
                emit commandMessage(tr("COPY 指定第二点："));
            }
            else
            {
                emit commandMessage(m_tool_mode == SToolMode::Move ? tr("MOVE 指定第二点：")
                                                                   : tr("ROTATE 指定旋转方向："));
            }
            update();
            return;
        }
        if (m_tool_mode == SToolMode::Scale && m_input_points.empty())
        {
            if (distance(*m_first_point, world_point) <= 1.0e-9)
            {
                emit commandMessage(tr("SCALE 参考长度必须大于零："));
                return;
            }
            m_input_points.push_back(world_point);
            emit commandMessage(tr("SCALE 指定新的参考长度点："));
            update();
            return;
        }
        if (m_tool_mode == SToolMode::Move)
        {
            completeMove(world_point);
        }
        else if (m_tool_mode == SToolMode::Copy)
        {
            completeCopy(world_point);
        }
        else if (m_tool_mode == SToolMode::Rotate)
        {
            completeRotate(world_point);
        }
        else if (m_tool_mode == SToolMode::Scale)
        {
            completeScale(world_point);
        }
        else
        {
            completeMirror(world_point);
        }
        return;
    }

    if (m_tool_mode == SToolMode::Polyline)
    {
        if (m_input_points.empty())
        {
            m_input_points.push_back(world_point);
            emit commandMessage(tr("PLINE 指定下一点或 [圆弧(A)/放弃(U)]："));
            update();
            return;
        }
        if (m_polyline_arc_mode && !m_polyline_arc_point)
        {
            m_polyline_arc_point = world_point;
            emit commandMessage(tr("PLINE 圆弧模式：指定圆弧端点："));
            update();
            return;
        }
        double bulge = 0.0;
        if (m_polyline_arc_mode &&
            !threePointBulge(m_input_points.back(), *m_polyline_arc_point, world_point, bulge))
        {
            emit commandMessage(tr("PLINE 三点共线，无法创建圆弧段，请重新指定弧上点："));
            m_polyline_arc_point.reset();
            update();
            return;
        }
        m_polyline_bulges.push_back(bulge);
        m_polyline_start_widths.push_back(m_polyline_start_width);
        m_polyline_end_widths.push_back(m_polyline_end_width);
        m_input_points.push_back(world_point);
        m_polyline_arc_point.reset();
        emit commandMessage(
            m_polyline_arc_mode
                ? tr("PLINE 圆弧模式：指定弧上点或 [直线(L)/闭合(C)/放弃(U)]：")
                : tr("PLINE 直线模式：指定下一点或 [圆弧(A)/闭合(C)/放弃(U)]，Enter 完成："));
        update();
        return;
    }

    if (m_tool_mode == SToolMode::Ellipse)
    {
        acceptEllipsePoint(world_point);
        return;
    }

    if (m_tool_mode == SToolMode::Spline)
    {
        acceptSplinePoint(world_point);
        return;
    }

    if (m_tool_mode == SToolMode::Circle)
    {
        acceptCircleConstructionPoint(world_point);
        return;
    }

    if (m_tool_mode == SToolMode::Arc)
    {
        acceptArcConstructionPoint(world_point);
        return;
    }

    if (m_tool_mode == SToolMode::StandardShape)
    {
        acceptStandardShapePoint(world_point);
        return;
    }

    if (m_tool_mode == SToolMode::Text)
    {
        if (!m_pending_text.trimmed().isEmpty())
        {
            auto transaction = m_document->beginTransaction(tr("创建文字"));
            transaction->addText(world_point, m_pending_text, 2.5, 0.0, m_pending_text_alignment,
                                 m_pending_text_vertical_alignment);
            transaction->commit();
            emit commandMessage(tr("TEXT 指定下一个插入点，Esc 结束："));
        }
        update();
        return;
    }

    if (dimensionTypeForToolMode(m_tool_mode))
    {
        acceptDimensionPoint(world_point);
        return;
    }

    if (m_tool_mode == SToolMode::Hatch)
    {
        const std::optional<SEntityId> boundary_id = entityAt(world_point);
        const SEntityRecord* boundary = boundary_id ? entityById(*boundary_id) : nullptr;
        if (boundary && boundary->type == SEntityType::Polyline)
        {
            const std::optional<SHatchEntity> hatch = hatchFromBoundaryEntity(*boundary);
            if (hatch)
            {
                auto transaction = m_document->beginTransaction(tr("创建填充"));
                transaction->addHatch(*hatch);
                transaction->commit();
                emit commandMessage(tr("HATCH 已创建，识别 %1 个岛。继续选择闭合边界：")
                                        .arg(hatch->island_boundaries.size()));
            }
            else
            {
                emit commandMessage(tr("HATCH 需要闭合多段线。"));
            }
        }
        else
        {
            emit commandMessage(tr("HATCH 未找到闭合多段线边界。"));
        }
        update();
        return;
    }

    if (!m_first_point.has_value())
    {
        m_first_point = world_point;
        if (m_tool_mode == SToolMode::Line)
        {
            m_command_start_point = world_point;
            emit commandMessage(tr("LINE 指定下一点或 [闭合(C)/放弃(U)]："));
        }
        else if (m_tool_mode == SToolMode::Rectangle)
        {
            emit commandMessage(tr("RECTANG 指定另一个角点："));
        }
        update();
        return;
    }

    auto transaction = m_document->beginTransaction(
        m_tool_mode == SToolMode::Line ? tr("创建直线") : tr("创建矩形"));
    if (m_tool_mode == SToolMode::Line)
    {
        transaction->addLine(*m_first_point, world_point);
        transaction->commit();
        m_first_point = world_point;
        ++m_line_segment_count;
        emit commandMessage(tr("LINE 指定下一点或 [闭合(C)/放弃(U)]，Esc 结束："));
    }
    else if (m_tool_mode == SToolMode::Rectangle)
    {
        std::vector<SPoint2d> vertices{
            *m_first_point,
            {world_point.x, m_first_point->y},
            world_point,
            {m_first_point->x, world_point.y},
        };
        transaction->addPolyline(std::move(vertices), true);
        transaction->commit();
        m_first_point.reset();
        emit commandMessage(tr("RECTANG 指定第一个角点："));
    }
    update();
}

} // namespace vectorPath
