#include "vp_cad_main_window.h"
#include "vp_cad_viewport.h"
#include "vp_cad_workspace_widget.h"
#include "vp_icon_provider.h"
#include "vp_theme_manager.h"

#include <DockWidget.h>
#include <DockWidgetTab.h>
#include <QAction>
#include <QKeySequence>
#include <QStatusBar>
#include <QToolButton>

namespace Vp
{

void VpCadMainWindow::refreshIcons()
{
    const VpDesignToken& tokens = m_theme_manager.tokens();
    const auto create_icon = [&tokens](VpIconType icon_type)
    {
        return VpIconProvider::createIcon(icon_type, tokens.text_primary, tokens.accent);
    };
    setWindowIcon(create_icon(VpIconType::Application));
    if (m_application_button)
    {
        m_application_button->setIcon(QIcon());
    }
    for (const auto& icon_action : m_icon_actions)
    {
        icon_action.first->setIcon(create_icon(icon_action.second));
    }
    for (const auto& icon_dock : m_icon_docks)
    {
        icon_dock.first->setIcon(create_icon(icon_dock.second));
        icon_dock.first->tabWidget()->setIconSize(QSize(22, 22));
    }
    for (const auto& icon_button : m_icon_status_buttons)
    {
        icon_button.first->setIcon(create_icon(icon_button.second));
    }
    const VpThemeMode theme_mode = m_theme_manager.themeMode();
    if (m_theme_action)
    {
        const VpIconType theme_icon =
            theme_mode == VpThemeMode::Dark
                ? VpIconType::ThemeDark
                : (theme_mode == VpThemeMode::Light ? VpIconType::ThemeLight
                                                    : VpIconType::ThemeHighContrast);
        m_theme_action->setIcon(create_icon(theme_icon));
    }
    if (m_dark_theme_action)
    {
        m_dark_theme_action->setChecked(theme_mode == VpThemeMode::Dark);
        m_light_theme_action->setChecked(theme_mode == VpThemeMode::Light);
        m_high_contrast_action->setChecked(theme_mode == VpThemeMode::HighContrast);
    }
    if (m_workspace)
    {
        const QColor grid_color = tokens.canvas.lighter(215);
        const QColor major_grid_color = tokens.canvas.lighter(360);
        m_workspace->viewport()->setAppearance(tokens.canvas, grid_color, major_grid_color,
                                               tokens.text_secondary);
    }
    updateGlobalColorActionIcon();
}

QAction* VpCadMainWindow::createAction(const QString& text, VpIconType icon_type,
                                       const QKeySequence& shortcut)
{
    const VpDesignToken& tokens = m_theme_manager.tokens();
    auto* action = new QAction(
        VpIconProvider::createIcon(icon_type, tokens.text_primary, tokens.accent), text, this);
    if (!shortcut.isEmpty())
    {
        action->setShortcut(shortcut);
    }
    action->setObjectName(
        QStringLiteral("smartAction_%1").arg(VpIconProvider::iconName(icon_type)));
    m_icon_actions.emplace_back(action, icon_type);
    return action;
}

QToolButton* VpCadMainWindow::createStatusButton(VpIconType icon_type, const QString& tool_tip,
                                                 bool is_enabled, bool is_checked)
{
    const VpDesignToken& tokens = m_theme_manager.tokens();
    auto* button = new QToolButton(statusBar());
    button->setObjectName(
        QStringLiteral("smartStatus_%1").arg(VpIconProvider::iconName(icon_type)));
    button->setIcon(VpIconProvider::createIcon(icon_type, tokens.text_primary, tokens.accent));
    button->setIconSize(QSize(22, 22));
    button->setFixedSize(30, 29);
    button->setAutoRaise(true);
    button->setCheckable(true);
    button->setChecked(is_checked);
    button->setEnabled(is_enabled);
    button->setToolTip(tool_tip);
    button->setAccessibleName(tool_tip);
    m_icon_status_buttons.emplace_back(button, icon_type);
    return button;
}

} // namespace Vp
