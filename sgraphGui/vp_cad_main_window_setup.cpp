#include "vp_cad_document.h"
#include "vp_cad_main_window.h"
#include "vp_cad_viewport.h"
#include "vp_cad_workspace_widget.h"
#include "vp_dialog_service.h"
#include "vp_drawing_settings.h"
#include "vp_geometry_types.h"
#include "vp_icon_provider.h"
#include "vp_svg_document_operations.h"
#include "vp_theme_manager.h"

#include <QAction>
#include <QActionGroup>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QSettings>
#include <QStatusBar>
#include <QToolButton>

namespace Vp
{

void VpCadMainWindow::createActions()
{
    m_new_action = createAction(tr("新建"), VpIconType::NewFile, QKeySequence::New);
    m_open_action = createAction(tr("打开"), VpIconType::OpenFile, QKeySequence::Open);
    m_save_action = createAction(tr("保存"), VpIconType::Save, QKeySequence::Save);
    m_save_as_action = createAction(tr("另存为"), VpIconType::SaveAs, QKeySequence::SaveAs);
    m_import_svg_action = createAction(tr("打开 SVG 为新图纸"), VpIconType::ImportSvg);
    m_import_bitmap_action = createAction(tr("打开位图为新图纸"), VpIconType::ImportBitmap);
    m_svg_fill_action = createAction(tr("SVG 填充"), VpIconType::SvgFill);
    m_svg_deduplicate_action = createAction(tr("去重"), VpIconType::SvgDeduplicate);
    auto* svg_deduplicate_menu = new QMenu(this);
    QAction* upper_first_action = createAction(tr("优先上层（正序）"), VpIconType::SvgDeduplicate);
    QAction* lower_first_action = createAction(tr("优先下层（逆序）"), VpIconType::SvgDeduplicate);
    svg_deduplicate_menu->addAction(upper_first_action);
    svg_deduplicate_menu->addAction(lower_first_action);
    m_svg_deduplicate_action->setMenu(svg_deduplicate_menu);
    m_export_dxf_action = createAction(tr("导出 DXF"), VpIconType::ExportDxf);
    m_export_dwg_action = createAction(tr("导出 DWG R2000"), VpIconType::ExportDwg);
    m_line_action = createAction(tr("直线"), VpIconType::DrawLine);
    m_line_action->setShortcut(QKeySequence(QStringLiteral("L")));
    registerToolAction(m_line_action, VpToolMode::Line);
    m_circle_action = createAction(tr("圆"), VpIconType::DrawCircle);
    registerToolAction(m_circle_action, VpToolMode::Circle);
    m_arc_action = createAction(tr("圆弧"), VpIconType::DrawArc);
    registerToolAction(m_arc_action, VpToolMode::Arc);
    m_ellipse_action = createAction(tr("椭圆"), VpIconType::DrawEllipse);
    registerToolAction(m_ellipse_action, VpToolMode::Ellipse);
    m_spline_action = createAction(tr("样条曲线"), VpIconType::DrawSpline);
    registerToolAction(m_spline_action, VpToolMode::Spline);
    m_undo_action = createAction(tr("撤销"), VpIconType::Undo, QKeySequence::Undo);
    m_redo_action = createAction(tr("重做"), VpIconType::Redo, QKeySequence::Redo);
    m_zoom_extents_action = createAction(tr("自动缩放"), VpIconType::ZoomExtents);
    m_zoom_extents_action->setToolTip(tr("自动适应全部可见实体"));
    m_theme_action = createAction(tr("主题"), VpIconType::Theme);
    m_dark_theme_action = createAction(tr("深色"), VpIconType::ThemeDark);
    m_light_theme_action = createAction(tr("浅色"), VpIconType::ThemeLight);
    m_high_contrast_action = createAction(tr("高对比度"), VpIconType::ThemeHighContrast);
    auto* theme_menu = new QMenu(this);
    auto* theme_group = new QActionGroup(this);
    theme_group->setExclusive(true);
    for (QAction* action : {m_dark_theme_action, m_light_theme_action, m_high_contrast_action})
    {
        action->setCheckable(true);
        theme_group->addAction(action);
        theme_menu->addAction(action);
    }
    m_theme_action->setMenu(theme_menu);

    registerShortcutAction(m_new_action, QStringLiteral("file.new"), QKeySequence::New);
    registerShortcutAction(m_open_action, QStringLiteral("file.open"), QKeySequence::Open);
    registerShortcutAction(m_save_action, QStringLiteral("file.save"), QKeySequence::Save);
    registerShortcutAction(m_save_as_action, QStringLiteral("file.save_as"), QKeySequence::SaveAs);
    registerShortcutAction(m_import_svg_action, QStringLiteral("file.import_svg"));
    registerShortcutAction(m_import_bitmap_action, QStringLiteral("file.import_bitmap"));
    registerShortcutAction(m_svg_fill_action, QStringLiteral("svg.fill"));
    registerShortcutAction(m_svg_deduplicate_action, QStringLiteral("svg.deduplicate"));
    registerShortcutAction(m_export_dxf_action, QStringLiteral("file.export_dxf"));
    registerShortcutAction(m_export_dwg_action, QStringLiteral("file.export_dwg"));
    registerShortcutAction(m_line_action, QStringLiteral("draw.line"),
                           QKeySequence(QStringLiteral("L")));
    registerShortcutAction(m_circle_action, QStringLiteral("draw.circle"));
    registerShortcutAction(m_arc_action, QStringLiteral("draw.arc"));
    registerShortcutAction(m_ellipse_action, QStringLiteral("draw.ellipse"));
    registerShortcutAction(m_spline_action, QStringLiteral("draw.spline"));
    registerShortcutAction(m_undo_action, QStringLiteral("edit.undo"), QKeySequence::Undo);
    registerShortcutAction(m_redo_action, QStringLiteral("edit.redo"), QKeySequence::Redo);
    registerShortcutAction(m_zoom_extents_action, QStringLiteral("view.zoom_extents"));
    registerShortcutAction(m_dark_theme_action, QStringLiteral("theme.dark"));
    registerShortcutAction(m_light_theme_action, QStringLiteral("theme.light"));
    registerShortcutAction(m_high_contrast_action, QStringLiteral("theme.high_contrast"));

    connect(m_new_action, &QAction::triggered, this, &VpCadMainWindow::newDocument);
    connect(m_open_action, &QAction::triggered, this, &VpCadMainWindow::openDocument);
    connect(m_save_action, &QAction::triggered, this, &VpCadMainWindow::saveDocument);
    connect(m_save_as_action, &QAction::triggered, this, &VpCadMainWindow::saveDocumentAs);
    connect(m_import_svg_action, &QAction::triggered, this, &VpCadMainWindow::openSvgAsNewDocument);
    connect(m_import_bitmap_action, &QAction::triggered, this,
            &VpCadMainWindow::openBitmapAsNewDocument);
    connect(m_svg_fill_action, &QAction::triggered, this, &VpCadMainWindow::fillImportedSvg);
    connect(m_svg_deduplicate_action, &QAction::triggered, this,
            [this]()
            {
                deduplicateImportedSvg(VpSvgLayerPriority::UpperFirst);
            });
    connect(upper_first_action, &QAction::triggered, this,
            [this]()
            {
                deduplicateImportedSvg(VpSvgLayerPriority::UpperFirst);
            });
    connect(lower_first_action, &QAction::triggered, this,
            [this]()
            {
                deduplicateImportedSvg(VpSvgLayerPriority::LowerFirst);
            });
    connect(m_export_dxf_action, &QAction::triggered, this, &VpCadMainWindow::exportDxf);
    connect(m_export_dwg_action, &QAction::triggered, this, &VpCadMainWindow::exportDwg);
    connect(m_line_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(VpToolMode::Line);
            });
    connect(m_circle_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(VpToolMode::Circle);
            });
    connect(m_arc_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(VpToolMode::Arc);
            });
    auto* circle_menu = new QMenu(this);
    const std::vector<std::pair<QString, VpCircleConstruction>> circle_methods{
        {tr("圆心、半径"), VpCircleConstruction::CenterRadius},
        {tr("圆心、直径"), VpCircleConstruction::CenterDiameter},
        {tr("两点圆"), VpCircleConstruction::TwoPoint},
        {tr("三点圆"), VpCircleConstruction::ThreePoint},
        {tr("相切、相切、半径"), VpCircleConstruction::TangentTangentRadius},
        {tr("相切、相切、相切"), VpCircleConstruction::TangentTangentTangent},
    };
    for (const auto& method : circle_methods)
    {
        QAction* method_action = createAction(method.first, VpIconType::DrawCircle);
        connect(method_action, &QAction::triggered, this,
                [this, construction = method.second]()
                {
                    m_workspace->viewport()->setCircleConstruction(construction);
                });
        circle_menu->addAction(method_action);
    }
    m_circle_action->setMenu(circle_menu);

    auto* arc_menu = new QMenu(this);
    const std::vector<std::pair<QString, VpArcConstruction>> arc_methods{
        {tr("三点圆弧"), VpArcConstruction::ThreePoint},
        {tr("圆心、起点、端点"), VpArcConstruction::CenterStartEnd},
        {tr("起点、圆心、端点"), VpArcConstruction::StartCenterEnd},
        {tr("起点、圆心、角度"), VpArcConstruction::StartCenterAngle},
        {tr("圆心、起点、角度"), VpArcConstruction::CenterStartAngle},
        {tr("起点、端点、角度"), VpArcConstruction::StartEndAngle},
        {tr("起点、端点、方向"), VpArcConstruction::StartEndDirection},
        {tr("起点、端点、半径"), VpArcConstruction::StartEndRadius},
    };
    for (const auto& method : arc_methods)
    {
        QAction* method_action = createAction(method.first, VpIconType::DrawArc);
        connect(method_action, &QAction::triggered, this,
                [this, construction = method.second]()
                {
                    m_workspace->viewport()->setArcConstruction(construction);
                });
        arc_menu->addAction(method_action);
    }
    m_arc_action->setMenu(arc_menu);
    connect(m_ellipse_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(VpToolMode::Ellipse);
            });
    connect(m_spline_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(VpToolMode::Spline);
            });
    connect(m_undo_action, &QAction::triggered, m_document.get(), &VpCadDocument::undo);
    connect(m_redo_action, &QAction::triggered, m_document.get(), &VpCadDocument::redo);
    connect(m_zoom_extents_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->zoomExtents();
            });
    connect(m_dark_theme_action, &QAction::triggered, this,
            [this]()
            {
                m_theme_manager.applyTheme(VpThemeMode::Dark);
            });
    connect(m_light_theme_action, &QAction::triggered, this,
            [this]()
            {
                m_theme_manager.applyTheme(VpThemeMode::Light);
            });
    connect(m_high_contrast_action, &QAction::triggered, this,
            [this]()
            {
                m_theme_manager.applyTheme(VpThemeMode::HighContrast);
            });
}

void VpCadMainWindow::createStatusBarWidgets()
{
    auto* status = new QStatusBar(this);
    status->setObjectName(QStringLiteral("smartCadStatusBar"));
    status->setMinimumHeight(31);
    status->setMaximumHeight(31);
    setStatusBar(status);
    auto* ready_label = new QLabel(tr("就绪"), status);
    ready_label->setObjectName(QStringLiteral("smartReadyLabel"));
    ready_label->setMinimumWidth(90);
    status->addWidget(ready_label);
    m_coordinate_label = new QLabel(tr("X 0.000   Y 0.000"), status);
    m_coordinate_label->setObjectName(QStringLiteral("smartCoordinateLabel"));
    m_coordinate_label->setAlignment(Qt::AlignCenter);
    status->addWidget(m_coordinate_label, 1);

    QToolButton* snap_button = createStatusButton(VpIconType::Snap, tr("捕捉设置"), true, true);
    snap_button->setCheckable(false);
    snap_button->setMenu(m_snap_action->menu());
    snap_button->setPopupMode(QToolButton::InstantPopup);
    QToolButton* grid_button =
        createStatusButton(VpIconType::Grid, tr("显示栅格（F7）"), true, true);
    QToolButton* ortho_button = createStatusButton(VpIconType::Ortho, tr("正交模式（F8）"), true);
    QToolButton* tracking_button =
        createStatusButton(VpIconType::Tracking, tr("对象追踪（F11）"), true, true);
    QToolButton* lineweight_button =
        createStatusButton(VpIconType::Lineweight, tr("显示线宽"), true);
    status->addPermanentWidget(snap_button);
    status->addPermanentWidget(grid_button);
    status->addPermanentWidget(ortho_button);
    status->addPermanentWidget(tracking_button);
    status->addPermanentWidget(lineweight_button);

    connect(grid_button, &QToolButton::toggled, m_workspace->viewport(),
            &VpCadViewport::setGridVisible);
    connect(ortho_button, &QToolButton::toggled, m_workspace->viewport(),
            &VpCadViewport::setOrthoEnabled);
    connect(tracking_button, &QToolButton::toggled, m_workspace->viewport(),
            &VpCadViewport::setTrackingEnabled);
    connect(lineweight_button, &QToolButton::toggled, m_workspace->viewport(),
            &VpCadViewport::setLineweightVisible);
    connect(m_workspace->viewport(), &VpCadViewport::gridVisibilityChanged, grid_button,
            &QToolButton::setChecked);
    connect(m_workspace->viewport(), &VpCadViewport::orthoChanged, ortho_button,
            &QToolButton::setChecked);
    connect(m_workspace->viewport(), &VpCadViewport::gridSnapChanged, m_grid_snap_action,
            &QAction::setChecked);
    connect(m_workspace->viewport(), &VpCadViewport::objectSnapModesChanged, this,
            [this](bool endpoint_enabled, bool center_enabled)
            {
                m_endpoint_snap_action->setChecked(endpoint_enabled);
                m_center_snap_action->setChecked(center_enabled);
            });
    connect(m_workspace->viewport(), &VpCadViewport::entityDisplayOptionsChanged, this,
            [this](bool nodes_visible, bool directions_visible, bool sequence_visible)
            {
                m_node_display_action->setChecked(nodes_visible);
                m_direction_display_action->setChecked(directions_visible);
                m_sequence_display_action->setChecked(sequence_visible);
                QSettings settings;
                settings.setValue(QStringLiteral("display/nodesVisible"), nodes_visible);
                settings.setValue(QStringLiteral("display/directionsVisible"), directions_visible);
                settings.setValue(QStringLiteral("display/sequenceVisible"), sequence_visible);
                settings.sync();
            });
    connect(m_workspace->viewport(), &VpCadViewport::trackingChanged, tracking_button,
            &QToolButton::setChecked);

    QAction* snap_shortcut = createAction(tr("对象捕捉"), VpIconType::Snap);
    QAction* grid_shortcut = createAction(tr("显示栅格"), VpIconType::Grid);
    QAction* ortho_shortcut = createAction(tr("正交模式"), VpIconType::Ortho);
    QAction* tracking_shortcut = createAction(tr("对象追踪"), VpIconType::Tracking);
    QAction* select_all_shortcut = createAction(tr("全选"), VpIconType::Select);
    QAction* delete_shortcut = createAction(tr("删除选择集"), VpIconType::Select);
    registerShortcutAction(snap_shortcut, QStringLiteral("precision.object_snap"),
                           QKeySequence(Qt::Key_F3));
    registerShortcutAction(grid_shortcut, QStringLiteral("precision.grid_visible"),
                           QKeySequence(Qt::Key_F7));
    registerShortcutAction(ortho_shortcut, QStringLiteral("precision.ortho"),
                           QKeySequence(Qt::Key_F8));
    registerShortcutAction(tracking_shortcut, QStringLiteral("precision.tracking"),
                           QKeySequence(Qt::Key_F11));
    registerShortcutAction(select_all_shortcut, QStringLiteral("selection.select_all"),
                           QKeySequence::SelectAll);
    registerShortcutAction(delete_shortcut, QStringLiteral("selection.delete"),
                           QKeySequence(Qt::Key_Delete));
    addAction(snap_shortcut);
    addAction(grid_shortcut);
    addAction(ortho_shortcut);
    addAction(tracking_shortcut);
    addAction(select_all_shortcut);
    addAction(delete_shortcut);
    connect(snap_shortcut, &QAction::triggered, this,
            [this]()
            {
                VpCadViewport* viewport = m_workspace->viewport();
                viewport->setObjectSnapEnabled(!viewport->isObjectSnapEnabled());
            });
    connect(grid_shortcut, &QAction::triggered, this,
            [this]()
            {
                VpCadViewport* viewport = m_workspace->viewport();
                viewport->setGridVisible(!viewport->isGridVisible());
            });
    connect(ortho_shortcut, &QAction::triggered, this,
            [this]()
            {
                VpCadViewport* viewport = m_workspace->viewport();
                viewport->setOrthoEnabled(!viewport->isOrthoEnabled());
            });
    connect(tracking_shortcut, &QAction::triggered, this,
            [this]()
            {
                VpCadViewport* viewport = m_workspace->viewport();
                viewport->setTrackingEnabled(!viewport->isTrackingEnabled());
            });
    connect(select_all_shortcut, &QAction::triggered, m_workspace->viewport(),
            &VpCadViewport::selectAll);
    connect(delete_shortcut, &QAction::triggered, m_workspace->viewport(),
            &VpCadViewport::deleteSelected);

    connect(m_workspace->viewport(), &VpCadViewport::cursorWorldPositionChanged, this,
            [this](const VpPoint2d& world_position)
            {
                m_last_cursor_x = world_position.x;
                m_last_cursor_y = world_position.y;
                updateCoordinateDisplay();
            });
    connect(m_document.get(), &VpCadDocument::drawingSettingsChanged, this,
            &VpCadMainWindow::updateCoordinateDisplay);
    updateCoordinateDisplay();
}

void VpCadMainWindow::updateCoordinateDisplay()
{
    const VpDrawingSettings& settings = m_document->drawingSettings();
    m_coordinate_label->setText(QStringLiteral("X %1   Y %2")
                                    .arg(formatLinearValue(m_last_cursor_x, settings),
                                         formatLinearValue(m_last_cursor_y, settings)));
}

void VpCadMainWindow::connectDocument()
{
    connect(m_document.get(), &VpCadDocument::documentChanged, m_workspace,
            &VpCadWorkspaceWidget::updateDocumentTitle);
    connect(m_document.get(), &VpCadDocument::modifiedChanged, this,
            [this](bool)
            {
                updateWindowTitle();
            });
    connect(m_document.get(), &VpCadDocument::filePathChanged, this,
            [this](const QString&)
            {
                updateWindowTitle();
            });
    connect(m_document.get(), &VpCadDocument::historyChanged, this,
            [this](bool can_undo, bool can_redo)
            {
                m_undo_action->setEnabled(can_undo);
                m_redo_action->setEnabled(can_redo);
            });
    m_undo_action->setEnabled(false);
    m_redo_action->setEnabled(false);
}

void VpCadMainWindow::beginTextCommand()
{
    bool is_accepted = false;
    const QString text = VpDialogService::getText(this, tr("创建文字"), tr("文字内容："),
                                                  QLineEdit::Normal, {}, &is_accepted);
    if (!is_accepted || text.trimmed().isEmpty())
    {
        return;
    }
    m_workspace->viewport()->setPendingText(text);
    m_workspace->viewport()->setToolMode(VpToolMode::Text);
}

void VpCadMainWindow::configureGripAction(QAction* grip_action)
{
    auto* menu = new QMenu(this);
    const auto add_mode = [this, menu](const QString& text, VpGripOperation operation)
    {
        QAction* action = createAction(text, VpIconType::Grip);
        connect(action, &QAction::triggered, this,
                [this, operation]()
                {
                    m_workspace->viewport()->beginGripEdit(operation);
                });
        menu->addAction(action);
    };
    add_mode(tr("夹点拉伸"), VpGripOperation::Stretch);
    add_mode(tr("夹点移动"), VpGripOperation::Move);
    add_mode(tr("夹点旋转"), VpGripOperation::Rotate);
    add_mode(tr("夹点缩放"), VpGripOperation::Scale);
    add_mode(tr("夹点镜像"), VpGripOperation::Mirror);
    grip_action->setMenu(menu);
    connect(grip_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->beginGripEdit(VpGripOperation::Stretch);
            });
}

} // namespace Vp
