#include "vp_cad_document.h"
#include "vp_cad_main_window.h"
#include "vp_cad_viewport.h"
#include "vp_cad_workspace_widget.h"
#include "vp_command_line_widget.h"
#include "vp_document_transaction.h"
#include "vp_entity.h"
#include "vp_icon_provider.h"
#include "vp_polygon_boolean.h"
#include "vp_qt_text.h"

#include <QAction>
#include <QMenu>
#include <QToolButton>
#include <SARibbonCategory.h>
#include <SARibbonPanel.h>
#include <algorithm>
#include <cmath>
#include <optional>

namespace Vp
{
namespace
{

std::optional<VpPolygonBooleanOperation> operationFromCommand(const QString& command)
{
    if (command == QLatin1String("UNION"))
    {
        return VpPolygonBooleanOperation::Union;
    }
    if (command == QLatin1String("INTERSECT"))
    {
        return VpPolygonBooleanOperation::Intersection;
    }
    if (command == QLatin1String("SUBTRACT"))
    {
        return VpPolygonBooleanOperation::Difference;
    }
    if (command == QLatin1String("XOR"))
    {
        return VpPolygonBooleanOperation::Xor;
    }
    if (command == QLatin1String("COMPLEMENT"))
    {
        return VpPolygonBooleanOperation::Complement;
    }
    return std::nullopt;
}

bool isStraightClosedPolyline(const VpEntityRecord& entity)
{
    if (entity.type != VpEntityType::Polyline)
    {
        return false;
    }
    const auto& polyline = std::get<VpPolylineEntity>(entity.geometry);
    return polyline.is_closed && polyline.vertices.size() >= 3 &&
           std::none_of(polyline.bulges.begin(), polyline.bulges.end(),
                        [](double bulge)
                        {
                            return std::abs(bulge) > 1.0e-12;
                        });
}

} // namespace

void VpCadMainWindow::configureBooleanPanel(SARibbonCategory* modify_category)
{
    SARibbonPanel* boolean_panel = modify_category->addPanel(tr("布尔运算"));
    QAction* boolean_action = createAction(tr("布尔运算"), VpIconType::BooleanUnion);
    boolean_action->setObjectName(QStringLiteral("smartBooleanAction"));
    boolean_action->setToolTip(tr("对所选闭合多段线执行布尔运算"));
    auto* boolean_menu = new QMenu(this);
    const struct VpBooleanActionSpec
    {
        const char* text;
        VpIconType icon_type;
        VpPolygonBooleanOperation operation;
        const char* command_id;
        const char* tool_tip;
    } boolean_specs[]{
        {"并集", VpIconType::BooleanUnion, VpPolygonBooleanOperation::Union, "modify.boolean_union",
         "合并所选闭合多段线"},
        {"交集", VpIconType::BooleanIntersection, VpPolygonBooleanOperation::Intersection,
         "modify.boolean_intersection", "保留所选闭合多段线的公共区域"},
        {"差集", VpIconType::BooleanDifference, VpPolygonBooleanOperation::Difference,
         "modify.boolean_difference", "从第一个选择轮廓减去其余轮廓"},
        {"异或", VpIconType::BooleanXor, VpPolygonBooleanOperation::Xor, "modify.boolean_xor",
         "保留不重叠的区域"},
        {"补集", VpIconType::BooleanComplement, VpPolygonBooleanOperation::Complement,
         "modify.boolean_complement", "从其余轮廓反向减去第一个选择轮廓"},
    };
    for (const VpBooleanActionSpec& boolean_spec : boolean_specs)
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
                executePolygonBoolean(VpPolygonBooleanOperation::Union);
            });
    boolean_panel->addLargeAction(boolean_action, QToolButton::MenuButtonPopup);
}

bool VpCadMainWindow::executeBooleanCommand(const QString& normalized_command)
{
    const std::optional<VpPolygonBooleanOperation> operation =
        operationFromCommand(normalized_command);
    if (!operation)
    {
        return false;
    }
    executePolygonBoolean(*operation);
    return true;
}

void VpCadMainWindow::executePolygonBoolean(VpPolygonBooleanOperation operation)
{
    const QVector<quint64> selected_ids = m_workspace->viewport()->selectedEntityIds();
    if (selected_ids.size() < 2)
    {
        m_command_line->appendMessage(tr("布尔运算需要选择至少两个闭合多段线。"));
        return;
    }

    VpPolygonPaths subject_paths;
    VpPolygonPaths clip_paths;
    std::vector<VpEntityRecord> selected_entities;
    selected_entities.reserve(static_cast<std::size_t>(selected_ids.size()));
    for (quint64 selected_id : selected_ids)
    {
        const auto entity_iterator =
            std::find_if(m_document->entities().begin(), m_document->entities().end(),
                         [selected_id](const VpEntityRecord& entity)
                         {
                             return entity.id == selected_id;
                         });
        if (entity_iterator == m_document->entities().end() ||
            !isStraightClosedPolyline(*entity_iterator))
        {
            m_command_line->appendMessage(tr("布尔运算只支持由直线段组成的闭合多段线。"));
            return;
        }
        selected_entities.push_back(*entity_iterator);
        const auto& polyline = std::get<VpPolylineEntity>(entity_iterator->geometry);
        if (selected_entities.size() == 1)
        {
            subject_paths.push_back(polyline.vertices);
        }
        else
        {
            clip_paths.push_back(polyline.vertices);
        }
    }

    const VpResult<VpPolygonPaths> boolean_result =
        polygonBoolean(subject_paths, clip_paths, operation);
    if (!boolean_result)
    {
        m_command_line->appendMessage(toQtError(boolean_result));
        return;
    }

    auto transaction = m_document->beginTransaction(tr("多段线布尔运算"));
    for (const VpEntityRecord& selected_entity : selected_entities)
    {
        transaction->removeEntity(selected_entity.id);
    }
    for (const VpPolygonPath& result_path : boolean_result.value())
    {
        VpEntityRecord result_entity = selected_entities.front();
        result_entity.geometry = VpPolylineEntity{result_path, true, {}, {}, {}};
        transaction->addEntityCopy(std::move(result_entity));
    }
    transaction->commit();
    m_workspace->viewport()->clearSelection();
    m_command_line->appendMessage(tr("布尔运算完成，生成 %1 个闭合轮廓。")
                                      .arg(static_cast<qulonglong>(boolean_result.value().size())));
}

} // namespace Vp
