#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_viewport_geometry.h"
#include "s_document_transaction.h"

#include <algorithm>
#include <cmath>

namespace vectorPath
{
namespace
{

bool isKeyword(const QString& normalized_keyword, const char* full_name, const char* alias)
{
    return normalized_keyword == QLatin1String(full_name) ||
           normalized_keyword == QLatin1String(alias);
}

} // namespace

void SCadViewport::showPolylineEditPrompt()
{
    if (m_selected_entity_ids.empty())
    {
        emit commandMessage(tr("PEDIT 选择一条或多条多段线："));
        return;
    }
    emit commandMessage(tr("PEDIT 已选择 %1 条多段线 "
                           "[闭合(C)/打开(O)/合并(J)/宽度(W)/反向(R)/去曲线(D)/放弃(U)/完成(X)]：")
                            .arg(m_selected_entity_ids.size()));
}

void SCadViewport::acceptPolylineEditPoint(const SPoint2d& world_point)
{
    const std::optional<SEntityId> picked_id = entityAt(world_point);
    const SEntityRecord* picked_entity = picked_id ? entityById(*picked_id) : nullptr;
    if (!picked_entity || picked_entity->type != SEntityType::Polyline)
    {
        emit commandMessage(tr("PEDIT 请选择多段线；直线可先用 JOIN 转换并合并。"));
        return;
    }
    const auto iterator =
        std::find(m_selected_entity_ids.begin(), m_selected_entity_ids.end(), picked_entity->id);
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
    showPolylineEditPrompt();
    update();
}

void SCadViewport::completePolylineEditJoin()
{
    if (!m_document || m_selected_entity_ids.size() < 2)
    {
        emit commandMessage(tr("PEDIT JOIN 至少需要两条首尾相连的开放多段线。"));
        return;
    }
    std::vector<SEntityRecord> sources;
    sources.reserve(m_selected_entity_ids.size());
    for (SEntityId entity_id : m_selected_entity_ids)
    {
        const SEntityRecord* source = entityById(entity_id);
        if (!source || source->type != SEntityType::Polyline)
        {
            emit commandMessage(tr("PEDIT JOIN 选择集中包含无效实体。"));
            return;
        }
        sources.push_back(*source);
    }
    SEntityRecord joined;
    if (!joinedEntity(sources, 1.0e-6, joined) || joined.type != SEntityType::Polyline)
    {
        emit commandMessage(tr("PEDIT JOIN 所选多段线的端点没有形成连续路径。"));
        return;
    }
    const SEntityId result_id = sources.front().id;
    auto transaction = m_document->beginTransaction(tr("PEDIT 合并多段线"));
    transaction->replaceEntity(result_id, std::move(joined));
    for (std::size_t index = 1; index < sources.size(); ++index)
    {
        transaction->removeEntity(sources[index].id);
    }
    transaction->commit();
    m_selected_entity_ids = {result_id};
    m_selected_entity_id = result_id;
    emitSelectionState();
    emit commandMessage(tr("PEDIT 已将 %1 条多段线合并为一条。").arg(sources.size()));
    showPolylineEditPrompt();
    update();
}

bool SCadViewport::handlePolylineEditKeyword(const QString& normalized_keyword)
{
    if (isKeyword(normalized_keyword, "DONE", "X") || normalized_keyword == QLatin1String("ENTER"))
    {
        m_tool_mode = SToolMode::Select;
        emit toolModeChanged(m_tool_mode);
        emit commandMessage(tr("PEDIT 已完成。"));
        update();
        return true;
    }
    if (isKeyword(normalized_keyword, "UNDO", "U"))
    {
        if (m_document && m_document->canUndo())
        {
            m_document->undo();
            emit commandMessage(tr("PEDIT 已放弃上一次编辑。"));
        }
        else
        {
            emit commandMessage(tr("PEDIT 没有可放弃的编辑。"));
        }
        showPolylineEditPrompt();
        return true;
    }
    if (isKeyword(normalized_keyword, "JOIN", "J"))
    {
        completePolylineEditJoin();
        return true;
    }
    if (m_selected_entity_ids.empty())
    {
        emit commandMessage(tr("PEDIT 请先选择多段线。"));
        return true;
    }

    const QStringList parts = normalized_keyword.split(QLatin1Char(' '), QString::SkipEmptyParts);
    const bool is_width = !parts.empty() && (parts.front() == QLatin1String("WIDTH") ||
                                             parts.front() == QLatin1String("W"));
    double width = 0.0;
    bool is_width_valid = false;
    if (is_width)
    {
        width = parts.size() == 2 ? parts[1].toDouble(&is_width_valid) : 0.0;
        if (!is_width_valid || !std::isfinite(width) || width < 0.0 || width > 1.0e9)
        {
            emit commandMessage(tr("PEDIT WIDTH 用法：WIDTH 非负宽度。"));
            return true;
        }
    }

    const bool is_close = isKeyword(normalized_keyword, "CLOSE", "C");
    const bool is_open = isKeyword(normalized_keyword, "OPEN", "O");
    const bool is_reverse = isKeyword(normalized_keyword, "REVERSE", "R");
    const bool is_decurve = isKeyword(normalized_keyword, "DECURVE", "D");
    if (!is_close && !is_open && !is_reverse && !is_decurve && !is_width)
    {
        return false;
    }

    auto transaction = m_document->beginTransaction(tr("PEDIT 编辑多段线"));
    int changed_count = 0;
    for (SEntityId entity_id : m_selected_entity_ids)
    {
        const SEntityRecord* source = entityById(entity_id);
        if (!source || source->type != SEntityType::Polyline)
        {
            continue;
        }
        const auto& source_polyline = std::get<SPolylineEntity>(source->geometry);
        SEntityRecord replacement;
        bool has_change = false;
        if (is_close && !source_polyline.is_closed)
        {
            has_change = polylineClosedStateEntity(*source, true, replacement);
        }
        else if (is_open && source_polyline.is_closed)
        {
            has_change = polylineClosedStateEntity(*source, false, replacement);
        }
        else if (is_reverse)
        {
            has_change = reversedPolylineEntity(*source, replacement);
        }
        else if (is_decurve &&
                 std::any_of(source_polyline.bulges.begin(), source_polyline.bulges.end(),
                             [](double bulge)
                             {
                                 return std::abs(bulge) > 1.0e-12;
                             }))
        {
            has_change = decurvedPolylineEntity(*source, replacement);
        }
        else if (is_width)
        {
            has_change = polylineConstantWidthEntity(*source, width, replacement);
        }
        if (has_change)
        {
            transaction->replaceEntity(entity_id, std::move(replacement));
            ++changed_count;
        }
    }
    if (changed_count > 0)
    {
        transaction->commit();
        emit commandMessage(tr("PEDIT 已更新 %1 条多段线。").arg(changed_count));
    }
    else
    {
        emit commandMessage(tr("PEDIT 当前选项没有产生可提交的变化。"));
    }
    showPolylineEditPrompt();
    update();
    return true;
}

} // namespace vectorPath
