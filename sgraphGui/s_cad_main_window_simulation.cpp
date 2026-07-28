#include "s_cad_main_window.h"

#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_workspace_widget.h"
#include "s_command_line_widget.h"
#include "s_icon_provider.h"
#include "s_toolpath_document.h"
#include "s_toolpath_sort_dialog.h"
#include "s_toolpath_simulation_controller.h"

#include <QAction>
#include <QDialog>
#include <SARibbonCategory.h>
#include <SARibbonPanel.h>

namespace smartGraphics
{
namespace
{

std::vector<SEntityId> selectedIds(const SCadViewport& viewport)
{
    std::vector<SEntityId> ids;
    const QVector<quint64> selected = viewport.selectedEntityIds();
    ids.reserve(static_cast<std::size_t>(selected.size()));
    for (quint64 id : selected)
    {
        ids.push_back(id);
    }
    return ids;
}

} // namespace

void SCadMainWindow::configureSimulationPanel(SARibbonCategory* simulation_category)
{
    SARibbonPanel* simulation_panel = simulation_category->addPanel(tr("仿真"));
    QAction* play_action = createAction(tr("启动/继续"), SIconType::Play);
    QAction* pause_action = createAction(tr("暂停"), SIconType::Pause);
    QAction* step_action = createAction(tr("单步"), SIconType::Step);
    QAction* speed_up_action = createAction(tr("加速"), SIconType::SpeedUp);
    QAction* speed_down_action = createAction(tr("减速"), SIconType::SpeedDown);
    QAction* trace_action = createAction(tr("显示轨迹"), SIconType::Simulation);
    play_action->setObjectName(QStringLiteral("smartSimulationPlayAction"));
    pause_action->setObjectName(QStringLiteral("smartSimulationPauseAction"));
    step_action->setObjectName(QStringLiteral("smartSimulationStepAction"));
    trace_action->setCheckable(true);
    trace_action->setChecked(true);
    registerShortcutAction(play_action, QStringLiteral("simulation.play"),
                           QKeySequence(QStringLiteral("Ctrl+F5")));
    registerShortcutAction(pause_action, QStringLiteral("simulation.pause"));
    registerShortcutAction(step_action, QStringLiteral("simulation.step"),
                           QKeySequence(QStringLiteral("F6")));
    registerShortcutAction(speed_up_action, QStringLiteral("simulation.speed_up"));
    registerShortcutAction(speed_down_action, QStringLiteral("simulation.speed_down"));
    connect(play_action, &QAction::triggered, this,
            [this]()
            {
                m_simulation_controller->play(selectedIds(*m_workspace->viewport()));
            });
    connect(pause_action, &QAction::triggered, this,
            [this]()
            {
                m_simulation_controller->pause();
            });
    connect(step_action, &QAction::triggered, this,
            [this]()
            {
                m_simulation_controller->step(selectedIds(*m_workspace->viewport()));
            });
    connect(speed_up_action, &QAction::triggered, this,
            [this]()
            {
                m_simulation_controller->speedUp();
            });
    connect(speed_down_action, &QAction::triggered, this,
            [this]()
            {
                m_simulation_controller->speedDown();
            });
    connect(trace_action, &QAction::toggled, this,
            [this](bool is_checked)
            {
                m_simulation_controller->setTraceVisible(is_checked);
            });
    simulation_panel->addLargeAction(play_action);
    simulation_panel->addLargeAction(pause_action);
    simulation_panel->addLargeAction(step_action);
    simulation_panel->addLargeAction(speed_up_action);
    simulation_panel->addLargeAction(speed_down_action);
    simulation_panel->addLargeAction(trace_action);

    SARibbonPanel* sort_panel = simulation_category->addPanel(tr("排序"));
    QAction* sort_action = createAction(tr("排序"), SIconType::Sort);
    sort_action->setObjectName(QStringLiteral("smartSortApplyAction"));
    connect(sort_action, &QAction::triggered, this,
            [this]()
            {
                SToolpathSortDialog dialog(m_sort_options, this);
                if (dialog.exec() != QDialog::Accepted)
                {
                    return;
                }
                m_sort_options = dialog.options();
                m_sort_options.shortest_start =
                    m_workspace->viewport()->visibleWorldBottomLeft();
                sortToolpaths(m_sort_options);
            });
    sort_panel->addLargeAction(sort_action);
}

void SCadMainWindow::sortToolpaths(const SToolpathSortOptions& options)
{
    const SToolpathDocumentSortResult result = sortDocumentToolpaths(
        *m_document, selectedIds(*m_workspace->viewport()), options);
    m_command_line->appendMessage(
        result.sorted_entity_count == 0
            ? tr("没有足够的可加工实体可供排序。")
            : tr("已排序 %1 个实体，反向 %2 个实体。")
                  .arg(result.sorted_entity_count)
                  .arg(result.reversed_entity_count));
}

bool SCadMainWindow::executeSimulationCommand(const QString& normalized_command)
{
    const std::vector<SEntityId> selection = selectedIds(*m_workspace->viewport());
    if (normalized_command == QLatin1String("SIMULATE") ||
        normalized_command == QLatin1String("SIM") ||
        normalized_command == QLatin1String("SIMPLAY") ||
        normalized_command == QLatin1String("TOOLPATH") || normalized_command == QLatin1String("TP"))
    {
        m_simulation_controller->play(selection);
        return true;
    }
    if (normalized_command == QLatin1String("SIMPAUSE"))
    {
        m_simulation_controller->pause();
        return true;
    }
    if (normalized_command == QLatin1String("SIMSTOP"))
    {
        m_simulation_controller->stop();
        return true;
    }
    if (normalized_command == QLatin1String("SIMSTEP"))
    {
        m_simulation_controller->step(selection);
        return true;
    }
    if (normalized_command == QLatin1String("SIMSPEED UP") ||
        normalized_command == QLatin1String("SIMSPEED DOWN"))
    {
        normalized_command.endsWith(QLatin1String("UP")) ? m_simulation_controller->speedUp()
                                                          : m_simulation_controller->speedDown();
        return true;
    }
    if (normalized_command == QLatin1String("SIMTRACE ON") ||
        normalized_command == QLatin1String("SIMTRACE OFF"))
    {
        m_simulation_controller->setTraceVisible(normalized_command.endsWith(QLatin1String("ON")));
        return true;
    }
    if (!normalized_command.startsWith(QLatin1String("TPSORT ")))
    {
        return false;
    }
    const QStringList parts = normalized_command.split(QLatin1Char(' '), QString::SkipEmptyParts);
    SToolpathSortOptions options;
    options.shortest_start = m_workspace->viewport()->visibleWorldBottomLeft();
    if (parts.size() == 3 && parts[1] == QLatin1String("SHORTEST"))
    {
        options.mode = SToolpathSortMode::Shortest;
        options.allow_reverse = parts[2] == QLatin1String("REVERSE");
    }
    else if (parts.size() == 3)
    {
        options.horizontal = parts[1] == QLatin1String("RIGHT")
                                 ? SToolpathHorizontalDirection::RightToLeft
                                 : SToolpathHorizontalDirection::LeftToRight;
        options.vertical = parts[2] == QLatin1String("BOTTOM")
                               ? SToolpathVerticalDirection::BottomToTop
                               : SToolpathVerticalDirection::TopToBottom;
    }
    else
    {
        m_command_line->appendMessage(tr("TPSORT 参数无效。"));
        return true;
    }
    sortToolpaths(options);
    return true;
}

} // namespace smartGraphics
