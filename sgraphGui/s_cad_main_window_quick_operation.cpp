#include "s_cad_main_window.h"

#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_workspace_widget.h"
#include "s_command_line_widget.h"
#include "s_icon_provider.h"
#include "s_quick_entity_operation.h"
#include "s_selection_context_bar.h"
#include "s_theme_manager.h"

#include <QAction>
#include <QColorDialog>
#include <QMenu>
#include <QPainter>
#include <QSettings>
#include <QToolBar>
#include <QToolButton>
#include <SARibbonCategory.h>
#include <SARibbonPanel.h>
#include <algorithm>

namespace vectorPath
{
namespace
{

std::vector<SEntityId> selectedIds(const SCadViewport& viewport)
{
    std::vector<SEntityId> result;
    for (quint64 entity_id : viewport.selectedEntityIds())
    {
        result.push_back(entity_id);
    }
    return result;
}

QColor firstSelectedColor(const SCadDocument& document,
                          const std::vector<SEntityId>& selected_entity_ids)
{
    for (const SEntityRecord& entity : document.entities())
    {
        if (std::find(selected_entity_ids.begin(), selected_entity_ids.end(), entity.id) !=
            selected_entity_ids.end())
        {
            return document.layerColor(entity.layer_name);
        }
    }
    return {};
}

} // namespace

void SCadMainWindow::configureQuickOperationPanel(SARibbonCategory* modify_category)
{
    const struct SActionSpec
    {
        const char* text;
        SIconType icon_type;
        SQuickEntityOperation operation;
        const char* command_id;
    } action_specs[]{
        {"反向", SIconType::QuickReverse, SQuickEntityOperation::Reverse, "quick.reverse"},
        {"顺90°", SIconType::RotateClockwise90, SQuickEntityOperation::RotateClockwise90,
         "quick.rotate_cw_90"},
        {"顺180°", SIconType::RotateClockwise180, SQuickEntityOperation::RotateClockwise180,
         "quick.rotate_cw_180"},
        {"逆90°", SIconType::RotateCounterclockwise90,
         SQuickEntityOperation::RotateCounterclockwise90,
         "quick.rotate_ccw_90"},
        {"逆180°", SIconType::RotateCounterclockwise180,
         SQuickEntityOperation::RotateCounterclockwise180,
         "quick.rotate_ccw_180"},
        {"垂直镜像", SIconType::MirrorVertical, SQuickEntityOperation::MirrorVertical,
         "quick.mirror_vertical"},
        {"水平镜像", SIconType::MirrorHorizontal, SQuickEntityOperation::MirrorHorizontal,
         "quick.mirror_horizontal"},
    };
    m_quick_operation_actions.clear();
    for (const SActionSpec& action_spec : action_specs)
    {
        QAction* action = createAction(tr(action_spec.text), action_spec.icon_type);
        registerShortcutAction(action, QString::fromLatin1(action_spec.command_id));
        connect(action, &QAction::triggered, this,
                [this, operation = action_spec.operation]()
                {
                    executeQuickEntityOperation(operation);
                });
        m_quick_operation_actions.push_back(action);
    }
    m_global_color_action = new QAction(tr("绘图颜色"), this);
    registerShortcutAction(m_global_color_action, QStringLiteral("quick.drawing_color"));
    connect(m_global_color_action, &QAction::triggered, this,
            &SCadMainWindow::chooseGlobalDrawingColor);
    const QColor initial_color(
        QSettings().value(QStringLiteral("drawing/globalColor")).toString());
    m_document->setPreferredDrawingColor(
        initial_color.isValid() ? initial_color : m_document->layerColor(m_document->currentLayerName()));
    updateGlobalColorActionIcon();

    QAction* menu_action = createAction(tr("快速变换"), SIconType::Rotate);
    auto* menu = new QMenu(this);
    for (int index = 0; index < static_cast<int>(m_quick_operation_actions.size()); ++index)
    {
        if (index == 5)
        {
            menu->addAction(m_global_color_action);
            menu->addSeparator();
        }
        menu->addAction(m_quick_operation_actions[static_cast<std::size_t>(index)]);
    }
    menu_action->setMenu(menu);
    SARibbonPanel* panel = modify_category->addPanel(tr("快速变换"));
    panel->addLargeAction(menu_action, QToolButton::InstantPopup);
}

void SCadMainWindow::configureQuickOperationBar()
{
    QToolBar* toolbar = m_workspace->quickOperationBar();
    for (int index = 0; index < static_cast<int>(m_quick_operation_actions.size()); ++index)
    {
        if (index == 5)
        {
            toolbar->addAction(m_global_color_action);
        }
        toolbar->addAction(m_quick_operation_actions[static_cast<std::size_t>(index)]);
    }
    toolbar->addSeparator();
    toolbar->addAction(m_zoom_extents_action);
    m_selection_context_bar = new SSelectionContextBar(this);
    QAction* color_action = createAction(tr("修改颜色"), SIconType::Layers);
    connect(color_action, &QAction::triggered, this, &SCadMainWindow::chooseSelectedEntityColor);
    m_selection_context_bar->addAction(color_action);
    connect(m_workspace->viewport(), &SCadViewport::selectionContextRequested, this,
            [this](const QPoint& global_position)
            {
                m_selection_context_bar->popupAt(global_position);
            });
}

void SCadMainWindow::executeQuickEntityOperation(SQuickEntityOperation operation)
{
    const std::vector<SEntityId> selected = selectedIds(*m_workspace->viewport());
    const std::vector<SEntityId> targets = quickOperationTargetIds(*m_document, selected);
    const std::optional<SPoint2d> center = m_workspace->viewport()->entitySetBoundsCenter(targets);
    if (targets.empty() || !center)
    {
        m_command_line->appendMessage(tr("没有可执行快速变换的实体。"));
        return;
    }
    const SQuickEntityOperationResult result =
        applyQuickEntityOperation(*m_document, selected, operation, *center);
    if (!selected.empty() && !result.created_entity_ids.empty())
    {
        m_workspace->viewport()->setSelectedEntityIds(result.created_entity_ids);
    }
    m_command_line->appendMessage(
        tr("快速变换已处理 %1 个实体。").arg(result.changed_count));
}

bool SCadMainWindow::executeQuickOperationCommand(const QString& normalized_command)
{
    const struct SCommandSpec
    {
        const char* command;
        SQuickEntityOperation operation;
    } command_specs[]{
        {"REVERSE", SQuickEntityOperation::Reverse},
        {"RV", SQuickEntityOperation::Reverse},
        {"ROTATE CW90", SQuickEntityOperation::RotateClockwise90},
        {"ROTATE CW180", SQuickEntityOperation::RotateClockwise180},
        {"ROTATE CCW90", SQuickEntityOperation::RotateCounterclockwise90},
        {"ROTATE CCW180", SQuickEntityOperation::RotateCounterclockwise180},
        {"MIRROR VERTICAL", SQuickEntityOperation::MirrorVertical},
        {"MIRROR HORIZONTAL", SQuickEntityOperation::MirrorHorizontal},
    };
    for (const SCommandSpec& command_spec : command_specs)
    {
        if (normalized_command == QLatin1String(command_spec.command))
        {
            executeQuickEntityOperation(command_spec.operation);
            return true;
        }
    }
    const auto parse_color = [&normalized_command](const QString& prefix)
    {
        if (!normalized_command.startsWith(prefix))
        {
            return QColor();
        }
        const QString value = normalized_command.mid(prefix.size()).trimmed();
        return value.size() == 7 && value.startsWith(QLatin1Char('#')) ? QColor(value) : QColor();
    };
    const QColor drawing_color = parse_color(QStringLiteral("DRAWCOLOR "));
    if (drawing_color.isValid())
    {
        m_document->setPreferredDrawingColor(drawing_color);
        QSettings settings;
        settings.setValue(QStringLiteral("drawing/globalColor"),
                          drawing_color.name(QColor::HexRgb));
        settings.sync();
        updateGlobalColorActionIcon();
        m_command_line->appendMessage(tr("全局绘图颜色已设置为 %1。").arg(drawing_color.name()));
        return true;
    }
    const QColor selection_color = parse_color(QStringLiteral("SELCOLOR "));
    if (selection_color.isValid())
    {
        const std::vector<SEntityId> selected = selectedIds(*m_workspace->viewport());
        const QString layer_name = m_document->assignEntitiesToColorLayer(selected, selection_color);
        m_command_line->appendMessage(layer_name.isEmpty() ? tr("选择集颜色未修改。")
                                                            : tr("选择集颜色已修改。"));
        return true;
    }
    if (normalized_command.startsWith(QLatin1String("DRAWCOLOR")) ||
        normalized_command.startsWith(QLatin1String("SELCOLOR")))
    {
        m_command_line->appendMessage(tr("颜色格式无效，请使用 #RRGGBB。"));
        return true;
    }
    return false;
}

void SCadMainWindow::chooseGlobalDrawingColor()
{
    QColorDialog dialog(m_document->preferredDrawingColor(), this);
    dialog.setWindowTitle(tr("设置全局绘图颜色"));
    dialog.setOption(QColorDialog::DontUseNativeDialog);
    dialog.setMinimumSize(720, 520);
    if (dialog.exec() != QColorDialog::Accepted || !dialog.currentColor().isValid())
    {
        return;
    }
    const QColor color = dialog.currentColor();
    m_document->setPreferredDrawingColor(color);
    QSettings settings;
    settings.setValue(QStringLiteral("drawing/globalColor"), color.name(QColor::HexRgb));
    settings.sync();
    updateGlobalColorActionIcon();
}

void SCadMainWindow::chooseSelectedEntityColor()
{
    const std::vector<SEntityId> selected = selectedIds(*m_workspace->viewport());
    const QColor initial_color = firstSelectedColor(*m_document, selected);
    if (selected.empty() || !initial_color.isValid())
    {
        return;
    }
    QColorDialog dialog(initial_color, this);
    dialog.setWindowTitle(tr("修改选择集颜色"));
    dialog.setOption(QColorDialog::DontUseNativeDialog);
    dialog.setMinimumSize(720, 520);
    if (dialog.exec() != QColorDialog::Accepted || !dialog.currentColor().isValid())
    {
        return;
    }
    const QString layer_name =
        m_document->assignEntitiesToColorLayer(selected, dialog.currentColor());
    m_command_line->appendMessage(layer_name.isEmpty() ? tr("选择集颜色未修改。")
                                                        : tr("选择集颜色已修改。"));
}

void SCadMainWindow::updateGlobalColorActionIcon()
{
    if (!m_global_color_action)
    {
        return;
    }
    QPixmap pixmap(64, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setPen(QPen(m_theme_manager.tokens().border, 2.0));
    painter.setBrush(m_document->preferredDrawingColor());
    painter.drawRoundedRect(QRectF(2, 2, 60, 28), 3, 3);
    m_global_color_action->setIcon(QIcon(pixmap));
    m_global_color_action->setToolTip(
        tr("全局绘图颜色：%1").arg(m_document->preferredDrawingColor().name()));
}

} // namespace vectorPath
