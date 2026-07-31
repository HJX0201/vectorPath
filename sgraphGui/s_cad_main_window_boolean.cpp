#include "s_cad_main_window.h"

#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_workspace_widget.h"
#include "s_command_line_widget.h"
#include "s_document_transaction.h"
#include "s_entity.h"
#include "s_icon_provider.h"
#include "s_polygon_boolean.h"

#include <QAction>
#include <QMenu>
#include <QToolButton>
#include <SARibbonCategory.h>
#include <SARibbonPanel.h>
#include <algorithm>
#include <cmath>
#include <optional>

namespace smartCam
{
namespace
{

std::optional<SPolygonBooleanOperation> operationFromCommand(const QString& command)
{
    if (command == QLatin1String("UNION"))
    {
        return SPolygonBooleanOperation::Union;
    }
    if (command == QLatin1String("INTERSECT"))
    {
        return SPolygonBooleanOperation::Intersection;
    }
    if (command == QLatin1String("SUBTRACT"))
    {
        return SPolygonBooleanOperation::Difference;
    }
    if (command == QLatin1String("XOR"))
    {
        return SPolygonBooleanOperation::Xor;
    }
    if (command == QLatin1String("COMPLEMENT"))
    {
        return SPolygonBooleanOperation::Complement;
    }
    return std::nullopt;
}

bool isStraightClosedPolyline(const SEntityRecord& entity)
{
    if (entity.type != SEntityType::Polyline)
    {
        return false;
    }
    const auto& polyline = std::get<SPolylineEntity>(entity.geometry);
    return polyline.is_closed && polyline.vertices.size() >= 3 &&
           std::none_of(polyline.bulges.begin(), polyline.bulges.end(),
                        [](double bulge)
                        {
                            return std::abs(bulge) > 1.0e-12;
                        });
}

} // namespace

void SCadMainWindow::configureBooleanPanel(SARibbonCategory* modify_category)
{
    SARibbonPanel* boolean_panel = modify_category->addPanel(tr("布尔运算"));
    QAction* boolean_action = createAction(tr("布尔运算"), SIconType::BooleanUnion);
    boolean_action->setObjectName(QStringLiteral("smartBooleanAction"));
    boolean_action->setToolTip(tr("对所选闭合多段线执行布尔运算"));
    auto* boolean_menu = new QMenu(this);
    const struct SBooleanActionSpec
    {
        const char* text;
        SIconType icon_type;
        SPolygonBooleanOperation operation;
        const char* command_id;
        const char* tool_tip;
    } boolean_specs[]{
        {"并集", SIconType::BooleanUnion, SPolygonBooleanOperation::Union, "modify.boolean_union",
         "合并所选闭合多段线"},
        {"交集", SIconType::BooleanIntersection, SPolygonBooleanOperation::Intersection,
         "modify.boolean_intersection", "保留所选闭合多段线的公共区域"},
        {"差集", SIconType::BooleanDifference, SPolygonBooleanOperation::Difference,
         "modify.boolean_difference", "从第一个选择轮廓减去其余轮廓"},
        {"异或", SIconType::BooleanXor, SPolygonBooleanOperation::Xor, "modify.boolean_xor",
         "保留不重叠的区域"},
        {"补集", SIconType::BooleanComplement, SPolygonBooleanOperation::Complement,
         "modify.boolean_complement", "从其余轮廓反向减去第一个选择轮廓"},
    };
    for (const SBooleanActionSpec& boolean_spec : boolean_specs)
    {
        QAction* operation_action = createAction(tr(boolean_spec.text), boolean_spec.icon_type);
        operation_action->setToolTip(tr(boolean_spec.tool_tip));
        registerShortcutAction(operation_action, QString::fromLatin1(boolean_spec.command_id));
        connect(operation_action, &QAction::triggered, this,
                [this, operation = boolean_spec.operation]()
                {
                    executePolygonBoolean(operation);
                });
        boolean_menu->addAction(operation_action);
    }
    boolean_action->setMenu(boolean_menu);
    connect(boolean_action, &QAction::triggered, this,
            [this]()
            {
                executePolygonBoolean(SPolygonBooleanOperation::Union);
            });
    boolean_panel->addLargeAction(boolean_action, QToolButton::MenuButtonPopup);
}

bool SCadMainWindow::executeBooleanCommand(const QString& normalized_command)
{
    const std::optional<SPolygonBooleanOperation> operation =
        operationFromCommand(normalized_command);
    if (!operation)
    {
        return false;
    }
    executePolygonBoolean(*operation);
    return true;
}

void SCadMainWindow::executePolygonBoolean(SPolygonBooleanOperation operation)
{
    const QVector<quint64> selected_ids = m_workspace->viewport()->selectedEntityIds();
    if (selected_ids.size() < 2)
    {
        m_command_line->appendMessage(tr("布尔运算需要选择至少两个闭合多段线。"));
        return;
    }

    SPolygonPaths subject_paths;
    SPolygonPaths clip_paths;
    std::vector<SEntityRecord> selected_entities;
    selected_entities.reserve(static_cast<std::size_t>(selected_ids.size()));
    for (quint64 selected_id : selected_ids)
    {
        const auto entity_iterator =
            std::find_if(m_document->entities().begin(), m_document->entities().end(),
                         [selected_id](const SEntityRecord& entity)
                         {
                             return entity.id == selected_id;
                         });
        if (entity_iterator == m_document->entities().end() ||
            !isStraightClosedPolyline(*entity_iterator))
        {
            m_command_line->appendMessage(
                tr("布尔运算只支持由直线段组成的闭合多段线。"));
            return;
        }
        selected_entities.push_back(*entity_iterator);
        const auto& polyline = std::get<SPolylineEntity>(entity_iterator->geometry);
        if (selected_entities.size() == 1)
        {
            subject_paths.push_back(polyline.vertices);
        }
        else
        {
            clip_paths.push_back(polyline.vertices);
        }
    }

    const SResult<SPolygonPaths> boolean_result =
        polygonBoolean(subject_paths, clip_paths, operation);
    if (!boolean_result)
    {
        m_command_line->appendMessage(boolean_result.errorMessage());
        return;
    }

    auto transaction = m_document->beginTransaction(tr("多段线布尔运算"));
    for (const SEntityRecord& selected_entity : selected_entities)
    {
        transaction->removeEntity(selected_entity.id);
    }
    for (const SPolygonPath& result_path : boolean_result.value())
    {
        SEntityRecord result_entity = selected_entities.front();
        result_entity.geometry = SPolylineEntity{result_path, true, {}, {}, {}};
        transaction->addEntityCopy(std::move(result_entity));
    }
    transaction->commit();
    m_workspace->viewport()->clearSelection();
    m_command_line->appendMessage(
        tr("布尔运算完成，生成 %1 个闭合轮廓。")
            .arg(static_cast<qulonglong>(boolean_result.value().size())));
}

} // namespace smartCam
