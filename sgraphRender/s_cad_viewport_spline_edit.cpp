#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_document_transaction.h"
#include "s_spline_edit.h"

#include <QPainter>
#include <algorithm>
#include <cmath>
#include <limits>

namespace smartGraphics
{
namespace
{

bool splineEditKeyword(const QString& keyword, const char* full_name, const char* alias)
{
    return keyword == QLatin1String(full_name) || keyword == QLatin1String(alias);
}

} // namespace

void SCadViewport::showSplineEditPrompt()
{
    if (!m_selected_entity_id)
    {
        emit commandMessage(tr("SPLINEDIT 选择样条曲线："));
        return;
    }
    if (m_spline_edit_control_index)
    {
        emit commandMessage(
            tr("SPLINEDIT 指定第 %1 个控制点的新位置：").arg(*m_spline_edit_control_index + 1));
        return;
    }
    emit commandMessage(tr("SPLINEDIT 拾取控制点，或 [移动(M)/反向(R)/转换多段线(P)/"
                           "放弃(U)/完成(X)]："));
}

void SCadViewport::acceptSplineEditPoint(const SPoint2d& world_point)
{
    if (!m_selected_entity_id)
    {
        const std::optional<SEntityId> picked_id = entityAt(world_point);
        const SEntityRecord* picked = picked_id ? entityById(*picked_id) : nullptr;
        if (!picked || picked->type != SEntityType::Spline)
        {
            emit commandMessage(tr("SPLINEDIT 请选择样条曲线。"));
            return;
        }
        m_selected_entity_id = picked->id;
        m_selected_entity_ids = {picked->id};
        emitSelectionState();
        showSplineEditPrompt();
        update();
        return;
    }
    const SEntityRecord* source = entityById(*m_selected_entity_id);
    if (!source || source->type != SEntityType::Spline)
    {
        m_selected_entity_id.reset();
        m_selected_entity_ids.clear();
        showSplineEditPrompt();
        return;
    }
    if (!m_spline_edit_control_index)
    {
        const auto& control_points = std::get<SSplineEntity>(source->geometry).control_points;
        std::size_t nearest_index = 0;
        double nearest_distance = std::numeric_limits<double>::max();
        for (std::size_t index = 0; index < control_points.size(); ++index)
        {
            const double candidate_distance = distance(control_points[index], world_point);
            if (candidate_distance < nearest_distance)
            {
                nearest_index = index;
                nearest_distance = candidate_distance;
            }
        }
        m_spline_edit_control_index = nearest_index;
        showSplineEditPrompt();
        update();
        return;
    }
    SEntityRecord replacement;
    if (!splineControlPointEntity(*source, *m_spline_edit_control_index, world_point, replacement))
    {
        emit commandMessage(tr("SPLINEDIT 控制点位置没有变化。"));
        return;
    }
    auto transaction = m_document->beginTransaction(tr("SPLINEDIT 移动控制点"));
    transaction->replaceEntity(source->id, std::move(replacement));
    transaction->commit();
    m_spline_edit_control_index.reset();
    showSplineEditPrompt();
    update();
}

bool SCadViewport::handleSplineEditKeyword(const QString& normalized_keyword)
{
    if (splineEditKeyword(normalized_keyword, "DONE", "X") ||
        normalized_keyword == QLatin1String("ENTER"))
    {
        m_spline_edit_control_index.reset();
        m_tool_mode = SToolMode::Select;
        emit toolModeChanged(m_tool_mode);
        emit commandMessage(tr("SPLINEDIT 已完成。"));
        update();
        return true;
    }
    if (splineEditKeyword(normalized_keyword, "UNDO", "U"))
    {
        m_spline_edit_control_index.reset();
        if (m_document && m_document->canUndo())
        {
            m_document->undo();
            emit commandMessage(tr("SPLINEDIT 已放弃上一次编辑。"));
        }
        else
        {
            emit commandMessage(tr("SPLINEDIT 没有可放弃的编辑。"));
        }
        showSplineEditPrompt();
        update();
        return true;
    }
    if (!m_selected_entity_id)
    {
        emit commandMessage(tr("SPLINEDIT 请先选择样条曲线。"));
        return true;
    }
    const SEntityRecord* source = entityById(*m_selected_entity_id);
    if (!source || source->type != SEntityType::Spline)
    {
        emit commandMessage(tr("SPLINEDIT 当前对象不再是样条曲线。"));
        return true;
    }

    const QStringList parts = normalized_keyword.split(QLatin1Char(' '), QString::SkipEmptyParts);
    const bool is_move =
        !parts.empty() &&
        (parts.front() == QLatin1String("MOVE") || parts.front() == QLatin1String("M") ||
         parts.front() == QLatin1String("CONTROL") || parts.front() == QLatin1String("CV"));
    if (is_move)
    {
        bool is_index_valid = false;
        const int one_based_index = parts.size() >= 2 ? parts[1].toInt(&is_index_valid) : 0;
        if (!is_index_valid || one_based_index < 1 || one_based_index > 4 ||
            (parts.size() != 2 && parts.size() != 4))
        {
            emit commandMessage(tr("SPLINEDIT MOVE 用法：MOVE 控制点序号(1-4) [X Y]。"));
            return true;
        }
        m_spline_edit_control_index = static_cast<std::size_t>(one_based_index - 1);
        if (parts.size() == 2)
        {
            showSplineEditPrompt();
            update();
            return true;
        }
        bool is_x_valid = false;
        bool is_y_valid = false;
        const double x = parts[2].toDouble(&is_x_valid);
        const double y = parts[3].toDouble(&is_y_valid);
        if (!is_x_valid || !is_y_valid || !std::isfinite(x) || !std::isfinite(y))
        {
            emit commandMessage(tr("SPLINEDIT MOVE 坐标无效。"));
            return true;
        }
        acceptSplineEditPoint({x, y});
        return true;
    }

    SEntityRecord replacement;
    QString transaction_label;
    bool changes_type = false;
    if (splineEditKeyword(normalized_keyword, "REVERSE", "R"))
    {
        reversedSplineEntity(*source, replacement);
        transaction_label = tr("SPLINEDIT 反向样条");
    }
    else if (!parts.empty() &&
             (parts.front() == QLatin1String("POLYLINE") || parts.front() == QLatin1String("P")))
    {
        bool is_precision_valid = parts.size() == 1;
        const int segment_count = parts.size() == 2 ? parts[1].toInt(&is_precision_valid) : 64;
        if (parts.size() > 2 || !is_precision_valid ||
            !splinePolylineEntity(*source, segment_count, replacement))
        {
            emit commandMessage(tr("SPLINEDIT POLYLINE 用法：POLYLINE 分段数(4-4096)。"));
            return true;
        }
        transaction_label = tr("SPLINEDIT 转换多段线");
        changes_type = true;
    }
    else
    {
        return false;
    }
    auto transaction = m_document->beginTransaction(transaction_label);
    transaction->replaceEntity(source->id, std::move(replacement));
    transaction->commit();
    if (changes_type)
    {
        m_tool_mode = SToolMode::Select;
        emit toolModeChanged(m_tool_mode);
        emit commandMessage(tr("SPLINEDIT 已转换为多段线，可用 PEDIT 继续编辑。"));
    }
    else
    {
        showSplineEditPrompt();
    }
    update();
    return true;
}

void SCadViewport::drawSplineEditPreview(QPainter& painter)
{
    if (!m_selected_entity_id)
    {
        return;
    }
    const SEntityRecord* source = entityById(*m_selected_entity_id);
    if (!source || source->type != SEntityType::Spline)
    {
        return;
    }
    SEntityRecord preview = *source;
    if (m_spline_edit_control_index)
    {
        splineControlPointEntity(*source, *m_spline_edit_control_index, m_cursor_world, preview);
        painter.setPen(QPen(QColor(92, 214, 255), 1.8, Qt::DashLine));
        drawEntityGeometry(painter, preview, QColor(92, 214, 255, 72));
    }
    const auto& control_points = std::get<SSplineEntity>(preview.geometry).control_points;
    painter.setPen(QPen(QColor(140, 155, 175, 210), 1.0, Qt::DashLine));
    for (std::size_t index = 1; index < control_points.size(); ++index)
    {
        painter.drawLine(worldToScreen(control_points[index - 1]),
                         worldToScreen(control_points[index]));
    }
    for (std::size_t index = 0; index < control_points.size(); ++index)
    {
        const QPointF screen_point = worldToScreen(control_points[index]);
        painter.setPen(QPen(QColor(235, 243, 255), 1.2));
        painter.setBrush(index == m_spline_edit_control_index.value_or(99) ? QColor(255, 183, 77)
                                                                           : QColor(38, 126, 212));
        painter.drawRect(QRectF(screen_point.x() - 4.0, screen_point.y() - 4.0, 8.0, 8.0));
        painter.drawText(screen_point + QPointF(6.0, -6.0), QString::number(index + 1));
    }
}

} // namespace smartGraphics
