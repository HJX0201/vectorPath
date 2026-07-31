#include "s_cad_main_window.h"

#include "s_cad_viewport.h"
#include "s_cad_workspace_widget.h"
#include "s_command_line_widget.h"
#include "s_icon_provider.h"
#include "s_theme_manager.h"

#include <QAction>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QToolButton>
#include <SARibbonCategory.h>
#include <SARibbonPanel.h>

namespace smartCam
{
namespace
{

QIcon toggleMenuCheckIcon(const QColor& color)
{
    QPixmap unchecked_pixmap(20, 20);
    unchecked_pixmap.fill(Qt::transparent);
    QPixmap checked_pixmap(20, 20);
    checked_pixmap.fill(Qt::transparent);
    QPainter painter(&checked_pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(color, 2.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawLine(QPointF(3.5, 10.5), QPointF(8.0, 15.0));
    painter.drawLine(QPointF(8.0, 15.0), QPointF(17.0, 5.5));
    QIcon icon;
    icon.addPixmap(unchecked_pixmap, QIcon::Normal, QIcon::Off);
    icon.addPixmap(checked_pixmap, QIcon::Normal, QIcon::On);
    return icon;
}

} // namespace

void SCadMainWindow::configureViewOptions(SARibbonCategory* view_category)
{
    SARibbonPanel* precision_panel = view_category->addPanel(tr("精确绘图"));
    QAction* coordinate_action = createAction(tr("精确坐标"), SIconType::CommandLine,
                                               QKeySequence(QStringLiteral("Ctrl+L")));
    coordinate_action->setToolTip(tr("输入 X,Y、@dx,dy 或 @距离<角度"));
    registerShortcutAction(coordinate_action, QStringLiteral("ui.command_line_focus"),
                           QKeySequence(QStringLiteral("Ctrl+L")));
    connect(coordinate_action, &QAction::triggered, this,
            [this]()
            {
                m_command_line->focusInput();
            });

    m_snap_action = createAction(tr("捕捉"), SIconType::Snap);
    m_snap_action->setToolTip(tr("设置可同时启用的捕捉类型"));
    auto* snap_menu = new QMenu(this);
    m_grid_snap_action = new QAction(tr("栅格捕捉"), this);
    m_center_snap_action = new QAction(tr("实体中心捕捉"), this);
    m_endpoint_snap_action = new QAction(tr("端点捕捉"), this);
    for (QAction* action : {m_grid_snap_action, m_center_snap_action, m_endpoint_snap_action})
    {
        action->setCheckable(true);
        snap_menu->addAction(action);
    }
    m_grid_snap_action->setChecked(false);
    m_center_snap_action->setChecked(true);
    m_endpoint_snap_action->setChecked(true);
    m_snap_action->setMenu(snap_menu);
    connect(m_grid_snap_action, &QAction::toggled, this,
            [this](bool is_enabled)
            {
                if (m_workspace)
                {
                    m_workspace->viewport()->setGridSnapEnabled(is_enabled);
                }
            });
    connect(m_center_snap_action, &QAction::toggled, this,
            [this](bool is_enabled)
            {
                if (m_workspace)
                {
                    m_workspace->viewport()->setCenterSnapEnabled(is_enabled);
                }
            });
    connect(m_endpoint_snap_action, &QAction::toggled, this,
            [this](bool is_enabled)
            {
                if (m_workspace)
                {
                    m_workspace->viewport()->setEndpointSnapEnabled(is_enabled);
                }
            });

    QAction* units_action = createAction(tr("图形单位"), SIconType::Settings);
    units_action->setToolTip(tr("设置插入单位、角度格式和显示精度"));
    registerShortcutAction(units_action, QStringLiteral("precision.units"));
    connect(units_action, &QAction::triggered, this, &SCadMainWindow::showUnitsDialog);
    precision_panel->addLargeAction(coordinate_action);
    precision_panel->addLargeAction(m_snap_action, QToolButton::InstantPopup);
    precision_panel->addLargeAction(units_action);

    SARibbonPanel* display_panel = view_category->addPanel(tr("显示"));
    QAction* display_action = createAction(tr("显示"), SIconType::Viewport);
    auto* display_menu = new QMenu(this);
    m_node_display_action = new QAction(tr("节点显示"), this);
    m_direction_display_action = new QAction(tr("方向显示"), this);
    m_sequence_display_action = new QAction(tr("序号显示"), this);
    for (QAction* action :
         {m_node_display_action, m_direction_display_action, m_sequence_display_action})
    {
        action->setCheckable(true);
        display_menu->addAction(action);
    }
    display_action->setMenu(display_menu);
    connect(m_node_display_action, &QAction::toggled, this,
            [this](bool is_visible)
            {
                if (m_workspace)
                {
                    m_workspace->viewport()->setNodeDisplayVisible(is_visible);
                }
            });
    connect(m_direction_display_action, &QAction::toggled, this,
            [this](bool is_visible)
            {
                if (m_workspace)
                {
                    m_workspace->viewport()->setDirectionDisplayVisible(is_visible);
                }
            });
    connect(m_sequence_display_action, &QAction::toggled, this,
            [this](bool is_visible)
            {
                if (m_workspace)
                {
                    m_workspace->viewport()->setSequenceDisplayVisible(is_visible);
                }
            });
    const auto refresh_check_icons = [this]()
    {
        const QIcon check_icon = toggleMenuCheckIcon(m_theme_manager.tokens().accent);
        for (QAction* action :
             {m_grid_snap_action, m_center_snap_action, m_endpoint_snap_action,
              m_node_display_action, m_direction_display_action, m_sequence_display_action})
        {
            action->setIcon(check_icon);
            action->setIconVisibleInMenu(true);
        }
    };
    refresh_check_icons();
    connect(&m_theme_manager, &SThemeManager::themeChanged, this,
            [refresh_check_icons](SThemeMode)
            {
                refresh_check_icons();
            });
    display_panel->addLargeAction(display_action, QToolButton::InstantPopup);
}

} // namespace smartCam
