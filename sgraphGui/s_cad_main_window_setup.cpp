#include "s_cad_document.h"
#include "s_cad_main_window.h"
#include "s_cad_viewport.h"
#include "s_cad_workspace_widget.h"
#include "s_drawing_settings.h"
#include "s_dialog_service.h"
#include "s_geometry_types.h"
#include "s_icon_provider.h"
#include "s_theme_manager.h"
#include "s_svg_document_operations.h"

#include <QAction>
#include <QActionGroup>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QSettings>
#include <QStatusBar>
#include <QToolButton>

namespace vectorPath
{

void SCadMainWindow::createActions()
{
    m_new_action = createAction(tr("新建"), SIconType::NewFile, QKeySequence::New);
    m_open_action = createAction(tr("打开"), SIconType::OpenFile, QKeySequence::Open);
    m_save_action = createAction(tr("保存"), SIconType::Save, QKeySequence::Save);
    m_save_as_action = createAction(tr("另存为"), SIconType::SaveAs, QKeySequence::SaveAs);
    m_import_svg_action = createAction(tr("打开 SVG 为新图纸"), SIconType::ImportSvg);
    m_import_bitmap_action =
        createAction(tr("打开位图为新图纸"), SIconType::ImportBitmap);
    m_svg_fill_action = createAction(tr("SVG 填充"), SIconType::SvgFill);
    m_svg_deduplicate_action =
        createAction(tr("去重"), SIconType::SvgDeduplicate);
    auto* svg_deduplicate_menu = new QMenu(this);
    QAction* upper_first_action =
        createAction(tr("优先上层（正序）"), SIconType::SvgDeduplicate);
    QAction* lower_first_action =
        createAction(tr("优先下层（逆序）"), SIconType::SvgDeduplicate);
    svg_deduplicate_menu->addAction(upper_first_action);
    svg_deduplicate_menu->addAction(lower_first_action);
    m_svg_deduplicate_action->setMenu(svg_deduplicate_menu);
    m_export_dxf_action = createAction(tr("导出 DXF"), SIconType::ExportDxf);
    m_export_dwg_action = createAction(tr("导出 DWG R2000"), SIconType::ExportDwg);
    m_line_action = createAction(tr("直线"), SIconType::DrawLine);
    m_line_action->setShortcut(QKeySequence(QStringLiteral("L")));
    registerToolAction(m_line_action, SToolMode::Line);
    m_circle_action = createAction(tr("圆"), SIconType::DrawCircle);
    registerToolAction(m_circle_action, SToolMode::Circle);
    m_arc_action = createAction(tr("圆弧"), SIconType::DrawArc);
    registerToolAction(m_arc_action, SToolMode::Arc);
    m_ellipse_action = createAction(tr("椭圆"), SIconType::DrawEllipse);
    registerToolAction(m_ellipse_action, SToolMode::Ellipse);
    m_spline_action = createAction(tr("样条曲线"), SIconType::DrawSpline);
    registerToolAction(m_spline_action, SToolMode::Spline);
    m_undo_action = createAction(tr("撤销"), SIconType::Undo, QKeySequence::Undo);
    m_redo_action = createAction(tr("重做"), SIconType::Redo, QKeySequence::Redo);
    m_zoom_extents_action = createAction(tr("自动缩放"), SIconType::ZoomExtents);
    m_zoom_extents_action->setToolTip(tr("自动适应全部可见实体"));
    m_theme_action = createAction(tr("主题"), SIconType::Theme);
    m_dark_theme_action = createAction(tr("深色"), SIconType::ThemeDark);
    m_light_theme_action = createAction(tr("浅色"), SIconType::ThemeLight);
    m_high_contrast_action = createAction(tr("高对比度"), SIconType::ThemeHighContrast);
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

    connect(m_new_action, &QAction::triggered, this, &SCadMainWindow::newDocument);
    connect(m_open_action, &QAction::triggered, this, &SCadMainWindow::openDocument);
    connect(m_save_action, &QAction::triggered, this, &SCadMainWindow::saveDocument);
    connect(m_save_as_action, &QAction::triggered, this, &SCadMainWindow::saveDocumentAs);
    connect(m_import_svg_action, &QAction::triggered, this,
            &SCadMainWindow::openSvgAsNewDocument);
    connect(m_import_bitmap_action, &QAction::triggered, this,
            &SCadMainWindow::openBitmapAsNewDocument);
    connect(m_svg_fill_action, &QAction::triggered, this,
            &SCadMainWindow::fillImportedSvg);
    connect(m_svg_deduplicate_action, &QAction::triggered, this,
            [this]()
            {
                deduplicateImportedSvg(SSvgLayerPriority::UpperFirst);
            });
    connect(upper_first_action, &QAction::triggered, this,
            [this]()
            {
                deduplicateImportedSvg(SSvgLayerPriority::UpperFirst);
            });
    connect(lower_first_action, &QAction::triggered, this,
            [this]()
            {
                deduplicateImportedSvg(SSvgLayerPriority::LowerFirst);
            });
    connect(m_export_dxf_action, &QAction::triggered, this, &SCadMainWindow::exportDxf);
    connect(m_export_dwg_action, &QAction::triggered, this, &SCadMainWindow::exportDwg);
    connect(m_line_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Line);
            });
    connect(m_circle_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Circle);
            });
    connect(m_arc_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Arc);
            });
    auto* circle_menu = new QMenu(this);
    const std::vector<std::pair<QString, SCircleConstruction>> circle_methods{
        {tr("圆心、半径"), SCircleConstruction::CenterRadius},
        {tr("圆心、直径"), SCircleConstruction::CenterDiameter},
        {tr("两点圆"), SCircleConstruction::TwoPoint},
        {tr("三点圆"), SCircleConstruction::ThreePoint},
        {tr("相切、相切、半径"), SCircleConstruction::TangentTangentRadius},
        {tr("相切、相切、相切"), SCircleConstruction::TangentTangentTangent},
    };
    for (const auto& method : circle_methods)
    {
        QAction* method_action = createAction(method.first, SIconType::DrawCircle);
        connect(method_action, &QAction::triggered, this,
                [this, construction = method.second]()
                {
                    m_workspace->viewport()->setCircleConstruction(construction);
                });
        circle_menu->addAction(method_action);
    }
    m_circle_action->setMenu(circle_menu);

    auto* arc_menu = new QMenu(this);
    const std::vector<std::pair<QString, SArcConstruction>> arc_methods{
        {tr("三点圆弧"), SArcConstruction::ThreePoint},
        {tr("圆心、起点、端点"), SArcConstruction::CenterStartEnd},
        {tr("起点、圆心、端点"), SArcConstruction::StartCenterEnd},
        {tr("起点、圆心、角度"), SArcConstruction::StartCenterAngle},
        {tr("圆心、起点、角度"), SArcConstruction::CenterStartAngle},
        {tr("起点、端点、角度"), SArcConstruction::StartEndAngle},
        {tr("起点、端点、方向"), SArcConstruction::StartEndDirection},
        {tr("起点、端点、半径"), SArcConstruction::StartEndRadius},
    };
    for (const auto& method : arc_methods)
    {
        QAction* method_action = createAction(method.first, SIconType::DrawArc);
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
                m_workspace->viewport()->setToolMode(SToolMode::Ellipse);
            });
    connect(m_spline_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Spline);
            });
    connect(m_undo_action, &QAction::triggered, m_document.get(), &SCadDocument::undo);
    connect(m_redo_action, &QAction::triggered, m_document.get(), &SCadDocument::redo);
    connect(m_zoom_extents_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->zoomExtents();
            });
    connect(m_dark_theme_action, &QAction::triggered, this,
            [this]()
            {
                m_theme_manager.applyTheme(SThemeMode::Dark);
            });
    connect(m_light_theme_action, &QAction::triggered, this,
            [this]()
            {
                m_theme_manager.applyTheme(SThemeMode::Light);
            });
    connect(m_high_contrast_action, &QAction::triggered, this,
            [this]()
            {
                m_theme_manager.applyTheme(SThemeMode::HighContrast);
            });
}

void SCadMainWindow::createStatusBarWidgets()
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

    QToolButton* snap_button = createStatusButton(SIconType::Snap, tr("捕捉设置"), true, true);
    snap_button->setCheckable(false);
    snap_button->setMenu(m_snap_action->menu());
    snap_button->setPopupMode(QToolButton::InstantPopup);
    QToolButton* grid_button =
        createStatusButton(SIconType::Grid, tr("显示栅格（F7）"), true, true);
    QToolButton* ortho_button = createStatusButton(SIconType::Ortho, tr("正交模式（F8）"), true);
    QToolButton* tracking_button =
        createStatusButton(SIconType::Tracking, tr("对象追踪（F11）"), true, true);
    QToolButton* lineweight_button =
        createStatusButton(SIconType::Lineweight, tr("显示线宽"), true);
    status->addPermanentWidget(snap_button);
    status->addPermanentWidget(grid_button);
    status->addPermanentWidget(ortho_button);
    status->addPermanentWidget(tracking_button);
    status->addPermanentWidget(lineweight_button);

    connect(grid_button, &QToolButton::toggled, m_workspace->viewport(),
            &SCadViewport::setGridVisible);
    connect(ortho_button, &QToolButton::toggled, m_workspace->viewport(),
            &SCadViewport::setOrthoEnabled);
    connect(tracking_button, &QToolButton::toggled, m_workspace->viewport(),
            &SCadViewport::setTrackingEnabled);
    connect(lineweight_button, &QToolButton::toggled, m_workspace->viewport(),
            &SCadViewport::setLineweightVisible);
    connect(m_workspace->viewport(), &SCadViewport::gridVisibilityChanged, grid_button,
            &QToolButton::setChecked);
    connect(m_workspace->viewport(), &SCadViewport::orthoChanged, ortho_button,
            &QToolButton::setChecked);
    connect(m_workspace->viewport(), &SCadViewport::gridSnapChanged, m_grid_snap_action,
            &QAction::setChecked);
    connect(m_workspace->viewport(), &SCadViewport::objectSnapModesChanged, this,
            [this](bool endpoint_enabled, bool center_enabled)
            {
                m_endpoint_snap_action->setChecked(endpoint_enabled);
                m_center_snap_action->setChecked(center_enabled);
            });
    connect(m_workspace->viewport(), &SCadViewport::entityDisplayOptionsChanged, this,
            [this](bool nodes_visible, bool directions_visible, bool sequence_visible)
            {
                m_node_display_action->setChecked(nodes_visible);
                m_direction_display_action->setChecked(directions_visible);
                m_sequence_display_action->setChecked(sequence_visible);
                QSettings settings;
                settings.setValue(QStringLiteral("display/nodesVisible"),
                                  nodes_visible);
                settings.setValue(QStringLiteral("display/directionsVisible"),
                                  directions_visible);
                settings.setValue(QStringLiteral("display/sequenceVisible"),
                                  sequence_visible);
                settings.sync();
            });
    connect(m_workspace->viewport(), &SCadViewport::trackingChanged, tracking_button,
            &QToolButton::setChecked);

    QAction* snap_shortcut = createAction(tr("对象捕捉"), SIconType::Snap);
    QAction* grid_shortcut = createAction(tr("显示栅格"), SIconType::Grid);
    QAction* ortho_shortcut = createAction(tr("正交模式"), SIconType::Ortho);
    QAction* tracking_shortcut = createAction(tr("对象追踪"), SIconType::Tracking);
    QAction* select_all_shortcut = createAction(tr("全选"), SIconType::Select);
    QAction* delete_shortcut = createAction(tr("删除选择集"), SIconType::Select);
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
                SCadViewport* viewport = m_workspace->viewport();
                viewport->setObjectSnapEnabled(!viewport->isObjectSnapEnabled());
            });
    connect(grid_shortcut, &QAction::triggered, this,
            [this]()
            {
                SCadViewport* viewport = m_workspace->viewport();
                viewport->setGridVisible(!viewport->isGridVisible());
            });
    connect(ortho_shortcut, &QAction::triggered, this,
            [this]()
            {
                SCadViewport* viewport = m_workspace->viewport();
                viewport->setOrthoEnabled(!viewport->isOrthoEnabled());
            });
    connect(tracking_shortcut, &QAction::triggered, this,
            [this]()
            {
                SCadViewport* viewport = m_workspace->viewport();
                viewport->setTrackingEnabled(!viewport->isTrackingEnabled());
            });
    connect(select_all_shortcut, &QAction::triggered, m_workspace->viewport(),
            &SCadViewport::selectAll);
    connect(delete_shortcut, &QAction::triggered, m_workspace->viewport(),
            &SCadViewport::deleteSelected);

    connect(m_workspace->viewport(), &SCadViewport::cursorWorldPositionChanged, this,
            [this](const SPoint2d& world_position)
            {
                m_last_cursor_x = world_position.x;
                m_last_cursor_y = world_position.y;
                updateCoordinateDisplay();
            });
    connect(m_document.get(), &SCadDocument::drawingSettingsChanged, this,
            &SCadMainWindow::updateCoordinateDisplay);
    updateCoordinateDisplay();
}

void SCadMainWindow::updateCoordinateDisplay()
{
    const SDrawingSettings& settings = m_document->drawingSettings();
    m_coordinate_label->setText(QStringLiteral("X %1   Y %2")
                                    .arg(formatLinearValue(m_last_cursor_x, settings),
                                         formatLinearValue(m_last_cursor_y, settings)));
}

void SCadMainWindow::connectDocument()
{
    connect(m_document.get(), &SCadDocument::documentChanged, m_workspace,
            &SCadWorkspaceWidget::updateDocumentTitle);
    connect(m_document.get(), &SCadDocument::modifiedChanged, this,
            [this](bool)
            {
                updateWindowTitle();
            });
    connect(m_document.get(), &SCadDocument::filePathChanged, this,
            [this](const QString&)
            {
                updateWindowTitle();
            });
    connect(m_document.get(), &SCadDocument::historyChanged, this,
            [this](bool can_undo, bool can_redo)
            {
                m_undo_action->setEnabled(can_undo);
                m_redo_action->setEnabled(can_redo);
            });
    m_undo_action->setEnabled(false);
    m_redo_action->setEnabled(false);
}

void SCadMainWindow::beginTextCommand()
{
    bool is_accepted = false;
    const QString text = SDialogService::getText(this, tr("创建文字"), tr("文字内容："),
                                                 QLineEdit::Normal, {}, &is_accepted);
    if (!is_accepted || text.trimmed().isEmpty())
    {
        return;
    }
    m_workspace->viewport()->setPendingText(text);
    m_workspace->viewport()->setToolMode(SToolMode::Text);
}

void SCadMainWindow::configureGripAction(QAction* grip_action)
{
    auto* menu = new QMenu(this);
    const auto add_mode = [this, menu](const QString& text, SGripOperation operation)
    {
        QAction* action = createAction(text, SIconType::Grip);
        connect(action, &QAction::triggered, this,
                [this, operation]()
                {
                    m_workspace->viewport()->beginGripEdit(operation);
                });
        menu->addAction(action);
    };
    add_mode(tr("夹点拉伸"), SGripOperation::Stretch);
    add_mode(tr("夹点移动"), SGripOperation::Move);
    add_mode(tr("夹点旋转"), SGripOperation::Rotate);
    add_mode(tr("夹点缩放"), SGripOperation::Scale);
    add_mode(tr("夹点镜像"), SGripOperation::Mirror);
    grip_action->setMenu(menu);
    connect(grip_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->beginGripEdit(SGripOperation::Stretch);
            });
}

} // namespace vectorPath
