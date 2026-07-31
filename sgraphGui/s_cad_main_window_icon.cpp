#include "s_cad_main_window.h"
#include "s_cad_viewport.h"
#include "s_cad_workspace_widget.h"
#include "s_icon_provider.h"
#include "s_theme_manager.h"

#include <DockWidget.h>
#include <DockWidgetTab.h>
#include <QAction>
#include <QKeySequence>
#include <QStatusBar>
#include <QToolButton>

namespace smartCam
{

void SCadMainWindow::refreshIcons()
{
    const SDesignToken& tokens = m_theme_manager.tokens();
    const auto create_icon = [&tokens](SIconType icon_type)
    {
        return SIconProvider::createIcon(icon_type, tokens.text_primary, tokens.accent);
    };
    setWindowIcon(create_icon(SIconType::Application));
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
    const SThemeMode theme_mode = m_theme_manager.themeMode();
    if (m_theme_action)
    {
        const SIconType theme_icon = theme_mode == SThemeMode::Dark
                                         ? SIconType::ThemeDark
                                         : (theme_mode == SThemeMode::Light
                                                ? SIconType::ThemeLight
                                                : SIconType::ThemeHighContrast);
        m_theme_action->setIcon(create_icon(theme_icon));
    }
    if (m_dark_theme_action)
    {
        m_dark_theme_action->setChecked(theme_mode == SThemeMode::Dark);
        m_light_theme_action->setChecked(theme_mode == SThemeMode::Light);
        m_high_contrast_action->setChecked(theme_mode == SThemeMode::HighContrast);
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

QAction* SCadMainWindow::createAction(const QString& text, SIconType icon_type,
                                      const QKeySequence& shortcut)
{
    const SDesignToken& tokens = m_theme_manager.tokens();
    auto* action = new QAction(
        SIconProvider::createIcon(icon_type, tokens.text_primary, tokens.accent), text, this);
    if (!shortcut.isEmpty())
    {
        action->setShortcut(shortcut);
    }
    action->setObjectName(QStringLiteral("smartAction_%1").arg(SIconProvider::iconName(icon_type)));
    m_icon_actions.emplace_back(action, icon_type);
    return action;
}

QToolButton* SCadMainWindow::createStatusButton(SIconType icon_type, const QString& tool_tip,
                                                bool is_enabled, bool is_checked)
{
    const SDesignToken& tokens = m_theme_manager.tokens();
    auto* button = new QToolButton(statusBar());
    button->setObjectName(QStringLiteral("smartStatus_%1").arg(SIconProvider::iconName(icon_type)));
    button->setIcon(SIconProvider::createIcon(icon_type, tokens.text_primary, tokens.accent));
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

} // namespace smartCam
