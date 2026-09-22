#include "vp_theme_manager.h"

#include "vp_proxy_style.h"

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPalette>
#include <QSettings>
#include <algorithm>

namespace Vp
{
namespace
{

QColor readColor(const QJsonObject& object, const char* key, const QColor& fallback)
{
    const QColor color(object.value(QLatin1String(key)).toString());
    return color.isValid() ? color : fallback;
}

} // namespace

VpThemeManager::VpThemeManager(QObject* parent) : QObject(parent)
{
    QSettings settings;
    const int saved_scale = settings.value(QStringLiteral("ui/scalePercent"), 100).toInt();
    m_ui_scale_percent = std::max(100, std::min(200, saved_scale));
}

VpThemeMode VpThemeManager::themeMode() const noexcept
{
    return m_theme_mode;
}

const VpDesignToken& VpThemeManager::tokens() const noexcept
{
    return m_tokens;
}

int VpThemeManager::uiScalePercent() const noexcept
{
    return m_ui_scale_percent;
}

bool VpThemeManager::setUiScalePercent(int scale_percent)
{
    const int bounded_percent = std::max(100, std::min(200, scale_percent));
    QSettings().setValue(QStringLiteral("ui/scalePercent"), bounded_percent);
    if (bounded_percent == m_ui_scale_percent)
    {
        return false;
    }
    m_ui_scale_percent = bounded_percent;
    applyPaletteAndStyle();
    emit uiScaleChanged(m_ui_scale_percent);
    return true;
}

bool VpThemeManager::applyTheme(VpThemeMode theme_mode)
{
    QString resource_path;
    switch (theme_mode)
    {
    case VpThemeMode::Light:
        resource_path = QStringLiteral(":/smartcad/themes/vp_theme_light.json");
        break;
    case VpThemeMode::Dark:
        resource_path = QStringLiteral(":/smartcad/themes/vp_theme_dark.json");
        break;
    case VpThemeMode::HighContrast:
        resource_path = QStringLiteral(":/smartcad/themes/vp_theme_high_contrast.json");
        break;
    }

    VpDesignToken new_tokens;
    if (!loadTokens(resource_path, new_tokens))
    {
        return false;
    }
    m_theme_mode = theme_mode;
    m_tokens = new_tokens;
    applyPaletteAndStyle();
    emit themeChanged(m_theme_mode);
    return true;
}

bool VpThemeManager::loadTokens(const QString& resource_path, VpDesignToken& tokens) const
{
    QFile file(resource_path);
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }
    const QJsonObject object = QJsonDocument::fromJson(file.readAll()).object();
    tokens.window = readColor(object, "window", QColor(24, 28, 36));
    tokens.surface = readColor(object, "surface", QColor(31, 36, 46));
    tokens.elevated_surface = readColor(object, "elevatedSurface", QColor(39, 45, 57));
    tokens.border = readColor(object, "border", QColor(62, 71, 88));
    tokens.text_primary = readColor(object, "textPrimary", QColor(236, 240, 246));
    tokens.text_secondary = readColor(object, "textSecondary", QColor(163, 174, 191));
    tokens.accent = readColor(object, "accent", QColor(48, 148, 255));
    tokens.accent_hover = readColor(object, "accentHover", QColor(78, 168, 255));
    tokens.danger = readColor(object, "danger", QColor(235, 91, 103));
    tokens.canvas = readColor(object, "canvas", QColor(12, 15, 20));
    return true;
}

void VpThemeManager::applyPaletteAndStyle()
{
    QApplication* application = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!application)
    {
        return;
    }

    if (dynamic_cast<VpProxyStyle*>(application->style()) == nullptr)
    {
        application->setStyle(new VpProxyStyle());
    }

    QPalette palette;
    palette.setColor(QPalette::Window, m_tokens.window);
    palette.setColor(QPalette::WindowText, m_tokens.text_primary);
    palette.setColor(QPalette::Base, m_tokens.surface);
    palette.setColor(QPalette::AlternateBase, m_tokens.elevated_surface);
    palette.setColor(QPalette::ToolTipBase, m_tokens.elevated_surface);
    palette.setColor(QPalette::ToolTipText, m_tokens.text_primary);
    palette.setColor(QPalette::Text, m_tokens.text_primary);
    palette.setColor(QPalette::Button, m_tokens.surface);
    palette.setColor(QPalette::ButtonText, m_tokens.text_primary);
    palette.setColor(QPalette::BrightText, Qt::white);
    palette.setColor(QPalette::Light, m_tokens.elevated_surface);
    palette.setColor(QPalette::Midlight, m_tokens.elevated_surface);
    palette.setColor(QPalette::Mid, m_tokens.border);
    palette.setColor(QPalette::Dark, m_tokens.border);
    palette.setColor(QPalette::Shadow, m_tokens.window.darker(135));
    palette.setColor(QPalette::Highlight, m_tokens.accent);
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::Text, m_tokens.text_secondary);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, m_tokens.text_secondary);
    application->setPalette(palette);

    QFont font(QStringLiteral("Segoe UI"));
    font.setPointSizeF(9.0 * static_cast<double>(m_ui_scale_percent) / 100.0);
    if (!QFontDatabase().families().contains(QStringLiteral("Segoe UI")))
    {
        font.setFamily(QStringLiteral("Microsoft YaHei UI"));
    }
    application->setFont(font);

    const QString hover_color = m_tokens.elevated_surface.lighter(112).name();
    const QString style_sheet = QStringLiteral(R"(
QToolTip { background:%1; color:%2; border:1px solid %3; padding:5px 7px; }
QLineEdit, QComboBox, QSpinBox { background:%4; color:%2; border:1px solid %3;
    border-radius:3px; padding:3px 6px; selection-background-color:%5; }
QLineEdit:focus, QComboBox:focus, QSpinBox:focus { border-color:%5; }
QLineEdit#smartCommandSearch { background:%4; color:%2; border:1px solid %3;
    border-radius:3px; padding:1px 7px; margin:0 5px 0 3px; }
QLineEdit#smartCommandSearch:hover { border-color:%7; }
QLineEdit#smartCommandSearch:focus { background:%1; border-color:%5; }
QLineEdit#smartCommandSearch + QListView { border:1px solid %3; }
QPushButton { background:%4; color:%2; border:1px solid %3; border-radius:3px;
    padding:4px 10px; }
QPushButton:hover, QToolButton:hover { background:%8; }
QMenu { background:%4; color:%2; border:1px solid %3; padding:4px; }
QMenu::item { padding:5px 28px 5px 9px; border-radius:2px; }
QMenu::item:selected { background:%5; color:white; }
QListView, QTreeView, QTextEdit, QTextBrowser { background:%4; color:%2;
    border:0; alternate-background-color:%1; }
QHeaderView::section { background:%6; color:%7; border:0; border-right:1px solid %3;
    border-bottom:1px solid %3; padding:4px 6px; }
QScrollBar:vertical { width:11px; background:%4; margin:0; }
QScrollBar::handle:vertical { min-height:24px; background:%3; border-radius:4px; margin:2px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }

SARibbonBar { background:%6; color:%2; border:0; border-bottom:1px solid %3; }
SARibbonQuickAccessBar, SARibbonButtonGroupWidget, SARibbonCtrlContainer {
    background:transparent; color:%2; }
SARibbonButtonGroupWidget > QToolButton { color:%2; background:transparent; border:0;
    padding:0 2px; }
SARibbonButtonGroupWidget > QToolButton:hover { background:%8; }
SARibbonApplicationButton { color:%7; background:transparent; border:0;
    border-bottom:2px solid transparent; padding:3px 10px 2px 10px; margin:0;
    min-height:28px; max-height:28px; font-size:inherit; }
SARibbonApplicationButton:hover { color:%2; background:%4; }
SARibbonApplicationButton::menu-indicator { width:0; }
SARibbonTabBar { background:transparent; }
SARibbonTabBar::tab { color:%7; background:transparent; border:0;
    border-bottom:2px solid transparent; min-width:48px; padding:3px 10px 2px 10px; margin:0; }
SARibbonTabBar::tab:hover:!selected { color:%2; background:%4; }
SARibbonTabBar::tab:selected { color:%2; background:%1; border-bottom-color:%5; }
SARibbonCategory { background:%1; color:%2; border-top:1px solid %3; }
SARibbonPanel { background:%1; color:%2; border:0; }
SARibbonPanelLabel { background:transparent; color:transparent; border:0; padding:0; }
SARibbonSeparatorWidget { color:%3; }
SARibbonToolButton { color:%2; background:transparent; border:1px solid transparent;
    border-radius:2px; }
SARibbonToolButton:hover { color:%2; background:%8; border-color:%3; }
SARibbonToolButton:pressed, SARibbonToolButton:checked { color:white; background:%5;
    border-color:%5; }

ads--CDockContainerWidget, ads--CDockAreaWidget { background:%6; }
ads--CDockContainerWidget ads--CDockSplitter::handle { background:%3; }
ads--CDockWidget { background:%4; color:%2; border:0; }
ads--CDockWidgetTab { background:%6; color:%7; border:0; min-height:30px; padding:0 3px; }
ads--CDockWidgetTab[activeTab="true"] { background:%4; border-bottom:2px solid %5; }
ads--CDockWidgetTab QLabel { color:%7; }
ads--CDockWidgetTab[activeTab="true"] QLabel { color:%2; }
ads--CTitleBarButton { background:transparent; border:0; padding:0; }
ads--CTitleBarButton:hover { background:%8; }
ads--CAutoHideTab { color:%7; background:%6; border:0; padding:2px; }
ads--CAutoHideTab:hover, ads--CAutoHideTab[activeTab="true"] { color:%5; }
ads--CAutoHideSideBar { background:%6; border:0; qproperty-spacing:8; }

QToolBar#smartDrawingToolBar { background:%6; border:0; border-right:1px solid %3;
    spacing:3px; padding:6px 5px; }
QToolBar#smartDrawingToolBar QToolButton { background:transparent; color:%2; border:0;
    border-radius:2px; min-width:52px; min-height:43px; padding:2px; }
QToolBar#smartDrawingToolBar QToolButton:hover { background:%8; }
QToolBar#smartDrawingToolBar QToolButton:pressed,
QToolBar#smartDrawingToolBar QToolButton:checked { background:%5; }
QToolBar#smartDrawingToolBar::separator { background:%3; height:1px; margin:4px 5px; }

QToolBar#smartQuickOperationBar { background:%6; border:0; border-top:1px solid %3;
    spacing:2px; padding:3px 5px; }
QToolBar#smartQuickOperationBar QToolButton { background:transparent; color:%2;
    border:1px solid transparent; border-radius:2px; padding:2px 5px; }
QToolBar#smartQuickOperationBar QToolButton:hover { background:%8; border-color:%3; }
QToolBar#smartQuickOperationBar QToolButton:pressed { color:white; background:%5; }
QToolBar#smartQuickOperationBar::separator { background:%3; width:1px; margin:3px 4px; }
QFrame#smartSelectionContextBar { background:%1; border:1px solid %3; border-radius:3px; }
QFrame#smartSelectionContextBar QToolButton { background:transparent; color:%2;
    border:1px solid transparent; border-radius:2px; padding:4px 7px; }
QFrame#smartSelectionContextBar QToolButton:hover { background:%8; border-color:%3; }
QFrame#smartSelectionContextBar QFrame { color:%3; }

QWidget#smartLayerPanel { background:%4; }
QTableWidget#smartLayerTable { background:%4; color:%2; border:1px solid %3; outline:0;
    gridline-color:%3; }
QTableWidget#smartLayerTable::item { min-height:25px; padding:0 4px; }
QTableWidget#smartLayerTable::item:hover { background:%1; }
QTableWidget#smartLayerTable::item:selected { background:%5; color:white; }
QTableWidget#smartLayerTable QHeaderView::section { background:%1; color:%7;
    border:0; border-right:1px solid %3; border-bottom:1px solid %3; padding:5px 6px; }

QTabWidget#smartDocumentTabs::pane { border:0; }
QTabBar#smartDocumentTabBar { background:%6; border-bottom:1px solid %3; }
QTabBar#smartDocumentTabBar::tab { color:%7; background:%6; border:0;
    border-right:1px solid %3; border-bottom:2px solid transparent;
    min-width:132px; padding:4px 12px 3px 12px; }
QTabBar#smartDocumentTabBar::tab:hover { color:%2; background:%4; }
QTabBar#smartDocumentTabBar::tab:selected { color:%2; background:%4; border-bottom-color:%5; }
QTabBar#smartSpaceTabs { background:%6; border-top:1px solid %3; }
QTabBar#smartSpaceTabs::tab { color:%7; background:%6; border:0; border-right:1px solid %3;
    min-width:78px; padding:3px 9px; }
QTabBar#smartSpaceTabs::tab:selected { color:%2; background:%4; border-top:2px solid %5; }
QWidget#smartCommandLine { background:%4; border:0; }
QTextEdit#smartCommandHistory { background:%4; color:%7; }
QLineEdit#smartCommandInput { background:%6; border:1px solid %3; border-radius:0; }
QStatusBar#smartCadStatusBar { background:%6; color:%7; border-top:1px solid %3; }
QStatusBar#smartCadStatusBar::item { border:0; }
QStatusBar#smartCadStatusBar QToolButton { background:transparent; border:0;
    border-left:1px solid %3; border-radius:0; padding:2px; }
    QStatusBar#smartCadStatusBar QToolButton:hover { background:%8; }
    QLabel#smartCoordinateLabel { padding-left:7px; }
    )")
                                    .arg(m_tokens.elevated_surface.name())
                                    .arg(m_tokens.text_primary.name())
                                    .arg(m_tokens.border.name())
                                    .arg(m_tokens.surface.name())
                                    .arg(m_tokens.accent.name())
                                    .arg(m_tokens.window.name())
                                    .arg(m_tokens.text_secondary.name())
                                    .arg(hover_color);
    const QString system_button_style =
        QStringLiteral(R"(
SARibbonSystemButtonBar { background:transparent; border:0; }
SARibbonSystemToolButton { background:transparent; border:0; padding:4px 12px; }
SARibbonSystemToolButton#SAMinimizeWindowButton:hover,
SARibbonSystemToolButton#SAMaximizeWindowButton:hover { background:%1; }
SARibbonSystemToolButton#SAMinimizeWindowButton:pressed,
SARibbonSystemToolButton#SAMaximizeWindowButton:pressed { background:%2; }
SARibbonSystemToolButton#SACloseWindowButton:hover { background:%3; }
SARibbonSystemToolButton#SACloseWindowButton:pressed { background:%4; }
)")
            .arg(hover_color, m_tokens.elevated_surface.lighter(124).name(), m_tokens.danger.name(),
                 m_tokens.danger.darker(118).name());
    application->setStyleSheet(style_sheet + system_button_style);
}

} // namespace Vp
