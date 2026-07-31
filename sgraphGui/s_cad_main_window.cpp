#include "s_cad_main_window.h"

#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_workspace_widget.h"
#include "s_command_line_widget.h"
#include "s_document_recovery_manager.h"
#include "s_document_transaction.h"
#include "s_dialog_service.h"
#include "s_dxf_codec.h"
#include "s_geometry_types.h"
#include "s_icon_provider.h"
#include "s_layer_record.h"
#include "s_shortcut_manager.h"
#include "s_theme_manager.h"
#include "s_toolpath_simulation_controller.h"

#include <DockAreaWidget.h>
#include <DockManager.h>
#include <DockWidget.h>
#include <QAction>
#include <QCloseEvent>
#include <QColorDialog>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QSettings>
#include <QShortcut>
#include <QStatusBar>
#include <QTextBrowser>
#include <QTextStream>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <SARibbonApplicationButton.h>
#include <SARibbonBar.h>
#include <SARibbonCategory.h>
#include <SARibbonPanel.h>
#include <SARibbonQuickAccessBar.h>
#include <SARibbonTabBar.h>
#include <algorithm>
#include <cmath>

namespace vectorPath
{
SCadMainWindow::SCadMainWindow(SThemeManager& theme_manager, QWidget* parent)
    : SARibbonMainWindow(
          parent, SARibbonMainWindowStyleFlag::UseRibbonMenuBar |
                      SARibbonMainWindowStyleFlag::UseNativeFrame),
      m_theme_manager(theme_manager),
      m_shortcut_manager(std::make_unique<SShortcutManager>()),
      m_document(std::make_unique<SCadDocument>())
{
    setObjectName(QStringLiteral("smartCadMainWindow"));
    setMinimumSize(1120, 720);
    resize(1440, 900);
    createActions();
    createRibbon();
    createDockingWorkspace();
    m_simulation_controller = std::make_unique<SToolpathSimulationController>(
        m_document.get(), m_workspace->viewport(), this);
    createStatusBarWidgets();
    connectDocument();
    m_recovery_manager = std::make_unique<SDocumentRecoveryManager>(m_document.get(), this);
    connect(m_recovery_manager.get(), &SDocumentRecoveryManager::recoveryFailed, this,
            [this](const QString& error_message)
            {
                m_command_line->appendMessage(tr("自动保存失败：%1").arg(error_message));
            });
    m_recovery_manager->start();
    connect(&m_theme_manager, &SThemeManager::themeChanged, this,
            [this](SThemeMode)
            {
                refreshIcons();
            });
    connect(&m_theme_manager, &SThemeManager::uiScaleChanged, this,
            &SCadMainWindow::applyUiScale);
    restoreWorkspace();
    applyUiScale(m_theme_manager.uiScalePercent());
    refreshIcons();
    updateWindowTitle();
}

SCadMainWindow::~SCadMainWindow()
{
    if (m_workspace && m_workspace->viewport())
    {
        QObject::disconnect(m_workspace->viewport(), &SCadViewport::toolModeChanged,
                            this, &SCadMainWindow::updateToolButtonHighlight);
    }
    m_tool_actions.clear();
    m_icon_actions.clear();
}

void SCadMainWindow::closeEvent(QCloseEvent* event)
{
    if (!maybeSave())
    {
        event->ignore();
        return;
    }
    saveWorkspace();
    event->accept();
}

void SCadMainWindow::createRibbon()
{
    SARibbonBar* ribbon = ribbonBar();
    ribbon->setContentsMargins(3, 0, 3, 0);
    ribbon->setRibbonStyle(SARibbonBar::RibbonStyleCompactThreeRow);
    ribbon->setTitleBarHeight(34);
    ribbon->setTabBarHeight(28);
    ribbon->setCategoryHeight(128);
    ribbon->setPanelTitleHeight(1);
    ribbon->quickAccessBar()->setIconSize(QSize(24, 24));
    ribbon->quickAccessBar()->hide();
    ribbon->setMinimumMode(false);
    ribbon->setTabDoubleClickToMinimumMode(false);

    m_application_button = ribbon->applicationButton();
    m_application_button->hide();

    auto* file_menu = new QMenu(this);
    file_menu->addAction(m_new_action);
    file_menu->addAction(m_open_action);
    file_menu->addAction(m_import_svg_action);
    file_menu->addAction(m_import_bitmap_action);
    file_menu->addAction(m_svg_fill_action);
    file_menu->addAction(m_svg_deduplicate_action);
    file_menu->addSeparator();
    file_menu->addAction(m_save_action);
    file_menu->addAction(m_save_as_action);
    file_menu->addAction(m_export_dxf_action);
    file_menu->addAction(m_export_dwg_action);

    SARibbonCategory* file_category = ribbon->addCategoryPage(tr("文件"));
    file_category->setObjectName(QStringLiteral("smartFileCategory"));
    file_category->addPanel(tr(""))->setVisible(false);
    SARibbonCategory* draw_category = ribbon->addCategoryPage(tr("绘图"));
    SARibbonCategory* modify_category = ribbon->addCategoryPage(tr("修改"));
    SARibbonCategory* annotate_category = ribbon->addCategoryPage(tr("注释"));
    SARibbonCategory* simulation_category = ribbon->addCategoryPage(tr("模拟"));
    SARibbonCategory* view_category = ribbon->addCategoryPage(tr("视图"));
    SARibbonCategory* help_category = ribbon->addCategoryPage(tr("帮助"));
    draw_category->setObjectName(QStringLiteral("smartDrawCategory"));
    modify_category->setObjectName(QStringLiteral("smartModifyCategory"));
    annotate_category->setObjectName(QStringLiteral("smartAnnotationCategory"));
    simulation_category->setObjectName(QStringLiteral("smartSimulationCategory"));
    view_category->setObjectName(QStringLiteral("smartViewCategory"));
    help_category->setObjectName(QStringLiteral("smartHelpCategory"));

    connect(ribbon, &SARibbonBar::currentRibbonTabChanged, this,
            [this, ribbon, file_menu](int index)
            {
                if (index == 0)
                {
                    QPoint popup_pos = ribbon->ribbonTabBar()->tabRect(0).bottomLeft();
                    popup_pos = ribbon->ribbonTabBar()->mapToGlobal(popup_pos);
                    file_menu->popup(popup_pos);
                    ribbon->setCurrentIndex(m_previous_tab_index);
                }
                else
                {
                    m_previous_tab_index = index;
                }
            });

    SARibbonPanel* draw_panel = draw_category->addPanel(tr("基础绘图"));
    draw_panel->addLargeAction(m_line_action);
    draw_panel->addLargeAction(m_circle_action, QToolButton::MenuButtonPopup);
    draw_panel->addLargeAction(m_ellipse_action);
    draw_panel->addLargeAction(m_spline_action);
    SARibbonPanel* svg_panel = draw_category->addPanel(tr("SVG"));
    svg_panel->addLargeAction(m_import_svg_action);
    svg_panel->addLargeAction(m_svg_fill_action);
    svg_panel->addLargeAction(m_svg_deduplicate_action,
                              QToolButton::MenuButtonPopup);
    QAction* polyline_action = createAction(tr("多段线"), SIconType::DrawPolyline);
    QAction* rectangle_action = createAction(tr("矩形"), SIconType::DrawRectangle);
    registerShortcutAction(polyline_action, QStringLiteral("draw.polyline"));
    registerShortcutAction(rectangle_action, QStringLiteral("draw.rectangle"));
    registerToolAction(polyline_action, SToolMode::Polyline);
    registerToolAction(rectangle_action, SToolMode::Rectangle);
    connect(polyline_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Polyline);
            });
    connect(rectangle_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Rectangle);
            });
    draw_panel->addLargeAction(polyline_action);
    draw_panel->addLargeAction(m_arc_action, QToolButton::MenuButtonPopup);
    draw_panel->addLargeAction(rectangle_action);
    const struct SShapeActionSpec
    {
        const char* text;
        SIconType icon_type;
        SStandardShapeType shape_type;
        const char* command_id;
    } shape_specs[]{
        {"五角星", SIconType::ShapeStar, SStandardShapeType::FivePointStar, "draw.star"},
        {"三角形", SIconType::ShapeTriangle, SStandardShapeType::Triangle, "draw.triangle"},
        {"五边形", SIconType::ShapePentagon, SStandardShapeType::Pentagon, "draw.pentagon"},
        {"六边形", SIconType::ShapeHexagon, SStandardShapeType::Hexagon, "draw.hexagon"},
        {"八边形", SIconType::ShapeOctagon, SStandardShapeType::Octagon, "draw.octagon"},
        {"菱形", SIconType::ShapeDiamond, SStandardShapeType::Diamond, "draw.diamond"},
    };
    for (const SShapeActionSpec& shape_spec : shape_specs)
    {
        QAction* shape_action = createAction(tr(shape_spec.text), shape_spec.icon_type);
        registerShortcutAction(shape_action, QString::fromLatin1(shape_spec.command_id));
        registerToolAction(shape_action, SToolMode::StandardShape);
        connect(shape_action, &QAction::triggered, this,
                [this, shape_type = shape_spec.shape_type]()
                {
                    m_workspace->viewport()->setStandardShapeType(shape_type);
                });
        draw_panel->addLargeAction(shape_action);
    }

    SARibbonPanel* modify_panel = modify_category->addPanel(tr("全部修改"));
    QAction* move_action = createAction(tr("移动"), SIconType::Move);
    QAction* copy_action = createAction(tr("复制"), SIconType::Copy);
    QAction* rotate_action = createAction(tr("旋转"), SIconType::Rotate);
    QAction* scale_action = createAction(tr("缩放"), SIconType::Scale);
    QAction* mirror_action = createAction(tr("镜像"), SIconType::Mirror);
    QAction* trim_action = createAction(tr("修剪"), SIconType::Trim);
    QAction* extend_action = createAction(tr("延伸"), SIconType::Extend);
    QAction* break_action = createAction(tr("打断"), SIconType::Break);
    QAction* join_action = createAction(tr("合并"), SIconType::Join);
    QAction* explode_action = createAction(tr("分解"), SIconType::Explode);
    QAction* stretch_action = createAction(tr("拉伸"), SIconType::Stretch);
    QAction* lengthen_action = createAction(tr("拉长"), SIconType::Lengthen);
    QAction* polyline_edit_action = createAction(tr("编辑多段线"), SIconType::PolylineEdit);
    QAction* spline_edit_action = createAction(tr("编辑样条"), SIconType::SplineEdit);
    QAction* grip_action = createAction(tr("夹点编辑"), SIconType::Grip);
    grip_action->setObjectName(QStringLiteral("smartGripAction"));
    QAction* fillet_action = createAction(tr("圆角"), SIconType::Fillet);
    QAction* chamfer_action = createAction(tr("倒角"), SIconType::Chamfer);
    QAction* blend_action = createAction(tr("混接曲线"), SIconType::Blend);
    QAction* align_action = createAction(tr("对齐"), SIconType::Align);
    QAction* array_rect_action = createAction(tr("矩形阵列"), SIconType::ArrayRect);
    QAction* array_polar_action = createAction(tr("环形阵列"), SIconType::ArrayPolar);
    QAction* array_path_action = createAction(tr("路径阵列"), SIconType::ArrayPath);
    QAction* array_edit_action = createAction(tr("编辑阵列"), SIconType::ArrayEdit);
    QAction* offset_action = createAction(tr("偏移"), SIconType::Offset);
    registerShortcutAction(move_action, QStringLiteral("modify.move"));
    registerShortcutAction(copy_action, QStringLiteral("modify.copy"));
    registerShortcutAction(rotate_action, QStringLiteral("modify.rotate"));
    registerShortcutAction(scale_action, QStringLiteral("modify.scale"));
    registerShortcutAction(mirror_action, QStringLiteral("modify.mirror"));
    registerShortcutAction(trim_action, QStringLiteral("modify.trim"));
    registerShortcutAction(extend_action, QStringLiteral("modify.extend"));
    registerShortcutAction(break_action, QStringLiteral("modify.break"));
    registerShortcutAction(join_action, QStringLiteral("modify.join"));
    registerShortcutAction(explode_action, QStringLiteral("modify.explode"));
    registerShortcutAction(stretch_action, QStringLiteral("modify.stretch"));
    registerShortcutAction(lengthen_action, QStringLiteral("modify.lengthen"));
    registerShortcutAction(polyline_edit_action, QStringLiteral("modify.polyline_edit"));
    registerShortcutAction(spline_edit_action, QStringLiteral("modify.spline_edit"));
    registerShortcutAction(grip_action, QStringLiteral("modify.grip"));
    registerShortcutAction(fillet_action, QStringLiteral("modify.fillet"));
    registerShortcutAction(chamfer_action, QStringLiteral("modify.chamfer"));
    registerShortcutAction(blend_action, QStringLiteral("modify.blend"));
    registerShortcutAction(align_action, QStringLiteral("modify.align"));
    registerShortcutAction(array_rect_action, QStringLiteral("modify.array_rect"));
    registerShortcutAction(array_polar_action, QStringLiteral("modify.array_polar"));
    registerShortcutAction(array_path_action, QStringLiteral("modify.array_path"));
    registerShortcutAction(array_edit_action, QStringLiteral("modify.array_edit"));
    registerShortcutAction(offset_action, QStringLiteral("modify.offset"));
    registerToolAction(move_action, SToolMode::Move);
    registerToolAction(copy_action, SToolMode::Copy);
    registerToolAction(rotate_action, SToolMode::Rotate);
    registerToolAction(scale_action, SToolMode::Scale);
    registerToolAction(mirror_action, SToolMode::Mirror);
    registerToolAction(offset_action, SToolMode::Offset);
    registerToolAction(trim_action, SToolMode::Trim);
    registerToolAction(extend_action, SToolMode::Extend);
    registerToolAction(break_action, SToolMode::Break);
    registerToolAction(join_action, SToolMode::Join);
    registerToolAction(explode_action, SToolMode::Explode);
    registerToolAction(stretch_action, SToolMode::Stretch);
    registerToolAction(lengthen_action, SToolMode::Lengthen);
    registerToolAction(polyline_edit_action, SToolMode::PolylineEdit);
    registerToolAction(spline_edit_action, SToolMode::SplineEdit);
    registerToolAction(fillet_action, SToolMode::Fillet);
    registerToolAction(chamfer_action, SToolMode::Chamfer);
    registerToolAction(blend_action, SToolMode::Blend);
    registerToolAction(align_action, SToolMode::Align);
    registerToolAction(array_rect_action, SToolMode::ArrayRect);
    registerToolAction(array_polar_action, SToolMode::ArrayPolar);
    registerToolAction(array_path_action, SToolMode::ArrayPath);
    registerToolAction(array_edit_action, SToolMode::ArrayEdit);
    connect(move_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Move);
            });
    connect(copy_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Copy);
            });
    connect(rotate_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Rotate);
            });
    connect(scale_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Scale);
            });
    connect(mirror_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Mirror);
            });
    connect(offset_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Offset);
            });
    connect(trim_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Trim);
            });
    connect(extend_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Extend);
            });
    connect(break_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Break);
            });
    connect(join_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Join);
            });
    connect(explode_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Explode);
            });
    connect(stretch_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Stretch);
            });
    connect(lengthen_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Lengthen);
            });
    connect(polyline_edit_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::PolylineEdit);
            });
    connect(spline_edit_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::SplineEdit);
            });
    configureGripAction(grip_action);
    connect(fillet_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Fillet);
            });
    connect(chamfer_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Chamfer);
            });
    connect(blend_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Blend);
            });
    connect(align_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Align);
            });
    connect(array_rect_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::ArrayRect);
            });
    connect(array_polar_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::ArrayPolar);
            });
    connect(array_path_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::ArrayPath);
            });
    connect(array_edit_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::ArrayEdit);
            });
    modify_panel->addLargeAction(move_action);
    modify_panel->addLargeAction(copy_action);
    modify_panel->addLargeAction(rotate_action);
    modify_panel->addLargeAction(scale_action);
    modify_panel->addLargeAction(mirror_action);
    modify_panel->addLargeAction(trim_action);
    modify_panel->addLargeAction(extend_action);
    modify_panel->addLargeAction(break_action);
    modify_panel->addLargeAction(join_action);
    modify_panel->addLargeAction(explode_action);
    modify_panel->addLargeAction(stretch_action);
    modify_panel->addLargeAction(lengthen_action);
    modify_panel->addLargeAction(polyline_edit_action);
    modify_panel->addLargeAction(spline_edit_action);
    modify_panel->addLargeAction(grip_action, QToolButton::MenuButtonPopup);
    modify_panel->addLargeAction(fillet_action);
    modify_panel->addLargeAction(chamfer_action);
    modify_panel->addLargeAction(blend_action);
    modify_panel->addLargeAction(align_action);
    modify_panel->addLargeAction(array_rect_action);
    modify_panel->addLargeAction(array_polar_action);
    modify_panel->addLargeAction(array_path_action);
    modify_panel->addLargeAction(array_edit_action);
    modify_panel->addLargeAction(offset_action);

    configureBooleanPanel(modify_category);
    configureQuickOperationPanel(modify_category);

    configureViewOptions(view_category);

    SARibbonPanel* annotation_panel = annotate_category->addPanel(tr("标注"));
    QAction* dimension_action = createAction(tr("线性标注"), SIconType::Dimension);
    QAction* text_action = createAction(tr("文字"), SIconType::Text);
    QAction* hatch_action = createAction(tr("填充"), SIconType::Hatch);
    registerShortcutAction(dimension_action, QStringLiteral("annotation.dimension_linear"));
    registerShortcutAction(text_action, QStringLiteral("annotation.text"));
    registerShortcutAction(hatch_action, QStringLiteral("annotation.hatch"));
    registerToolAction(dimension_action, SToolMode::LinearDimension);
    registerToolAction(text_action, SToolMode::Text);
    registerToolAction(hatch_action, SToolMode::Hatch);
    connect(dimension_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::LinearDimension);
            });
    connect(text_action, &QAction::triggered, this, &SCadMainWindow::beginTextCommand);
    connect(hatch_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Hatch);
            });
    annotation_panel->addLargeAction(dimension_action);
    configureDimensionPanel(annotation_panel);
    annotation_panel->addLargeAction(text_action);
    annotation_panel->addLargeAction(hatch_action);
    configureHatchPanel(annotation_panel);
    configureAnnotationPanel(annotation_panel);

    configureSimulationPanel(simulation_category);

    SARibbonPanel* theme_panel = help_category->addPanel(tr("主题"));
    theme_panel->addLargeAction(m_theme_action, QToolButton::InstantPopup);

    SARibbonPanel* recovery_panel = help_category->addPanel(tr("图形维护"));
    QAction* recovery_action = createAction(tr("图形恢复"), SIconType::Recovery);
    QAction* recovery_copy_action = createAction(tr("立即自动保存"), SIconType::Save);
    QAction* audit_action = createAction(tr("审计并修复"), SIconType::Audit);
    registerShortcutAction(recovery_action, QStringLiteral("file.recovery"));
    registerShortcutAction(recovery_copy_action, QStringLiteral("file.autosave"));
    registerShortcutAction(audit_action, QStringLiteral("file.audit"));
    connect(recovery_action, &QAction::triggered, this, &SCadMainWindow::showRecoveryManager);
    connect(recovery_copy_action, &QAction::triggered, this, &SCadMainWindow::createRecoveryCopy);
    connect(audit_action, &QAction::triggered, this,
            [this]()
            {
                runDocumentAudit(true);
            });
    recovery_panel->addLargeAction(recovery_action);
    recovery_panel->addLargeAction(recovery_copy_action);
    recovery_panel->addLargeAction(audit_action);

    SARibbonPanel* extension_panel = help_category->addPanel(tr("扩展"));
    QAction* settings_action = createAction(tr("界面比例"), SIconType::UiScale);
    QAction* shortcut_action = createAction(tr("修改快捷键"), SIconType::Settings);
    shortcut_action->setObjectName(QStringLiteral("smartModifyShortcutsAction"));
    connect(settings_action, &QAction::triggered, this, &SCadMainWindow::showInterfaceSettings);
    registerShortcutAction(shortcut_action, QStringLiteral("ui.shortcuts"));
    connect(shortcut_action, &QAction::triggered, this, &SCadMainWindow::showShortcutSettings);
    extension_panel->addLargeAction(settings_action);
    extension_panel->addLargeAction(shortcut_action);

    SARibbonPanel* help_panel = help_category->addPanel(tr("帮助"));
    QAction* help_action =
        createAction(tr("使用帮助"), SIconType::Help, QKeySequence::HelpContents);
    registerShortcutAction(help_action, QStringLiteral("help.contents"),
                           QKeySequence::HelpContents);
    connect(
        help_action, &QAction::triggered, this,
        [this]()
        {
            SDialogService::information(
                this, tr("vectorPath 使用帮助"),
                tr("绘图：LINE/L、CIRCLE/C、ELLIPSE/EL、SPLINE/SPL、PLINE/PL、ARC/A、RECTANG/REC\n"
                   "标准图形：STAR、TRIANGLE、PENTAGON、HEXAGON、OCTAGON、DIAMOND\n"
                   "修改：MOVE/M、COPY/CO、ROTATE/RO、SCALE/SC、MIRROR/MI、"
                   "TRIM/TR、EXTEND/EX、BREAK/BR、JOIN/J、EXPLODE/X、"
                   "STRETCH/S、LENGTHEN/LEN、PEDIT/PE、SPLINEDIT/SPE、"
                   "GRIP_STRETCH/GRIP、GRIP_MOVE/GM、"
                   "GRIP_ROTATE/GR、GRIP_SCALE/GS、GRIP_MIRROR/GMI、FILLET/F、"
                   "CHAMFER/CHA、BLEND/BL、ALIGN/AL、ARRAYRECT/AR、"
                   "ARRAYPOLAR/AP、ARRAYPATH/PA、ARRAYEDIT/AE、OFFSET/O、ERASE/E；"
                   "闭合多段线布尔运算：UNION、INTERSECT、SUBTRACT、XOR、COMPLEMENT\n"
                   "注释：TEXT/T、DIMLINEAR/DLI、HATCH/H\n"
                   "视图：ZOOM EXTENTS/ZE、GRID ON/OFF、OSNAP ON/OFF、ORTHO ON/OFF\n"
                   "精确：SNAP ON/OFF、UNITS、ANGLEFORMAT\n"
                   "文件：NEW、OPEN、SAVE、IMPORTSVG/SVGIMPORT、"
                   "IMPORTBITMAP/BITMAPIMPORT/IMAGEIMPORT、DXFOUT、"
                   "RECOVER、AUTOSAVE、AUDIT\n\n"
                   "SVG：SVGFILL 独立生成线填充；SVGDEDUP UPPER "
                   "优先上层正序去重，SVGDEDUP LOWER 优先下层逆序去重。\n\n"
                   "图层：LAYER MERGECOLOR 合并同色图层；属性页修改实体颜色时会自动"
                   "建立独立颜色图层。\n\n"
                   "快捷键设置：帮助 → 修改快捷键，或输入 CUI、SHORTCUTS、KEYBOARD；"
                   "支持搜索、冲突检测、清除和恢复默认。\n\n"
                   "快捷键：Delete 删除选择集，Ctrl+A 全选，Esc 取消/清除选择，"
                   "Space/Enter 重复上一命令，F3 捕捉，F7 栅格，F8 正交，"
                   "F9 栅格捕捉，F11 对象追踪；"
                   "夹点拖动时 Space 循环拉伸/移动/旋转/缩放/镜像；命令行 Tab 补全、"
                   "Up/Down 浏览历史。\n\n"
                   "绘图命令启动后可输入绝对 X,Y、相对 @dx,dy 或相对极坐标 "
                   "@距离<角度；输入时视口实时预览。PLINE 可用 A/L 切换圆弧/直线、"
                   "C 闭合、U 放弃点；CIRCLE 支持 2P/3P/DIAMETER/TTR/TTT，ARC 支持 "
                   "CSE/SCE/SCA/CSA/SEA/SED/SER，"
                   "LINE 可用 C 闭合、U 放弃线段。"));
        });
    help_panel->addLargeAction(help_action);

    ribbon->setPanelToolButtonIconSize(QSize(28, 28), QSize(28, 28));
    ribbon->setCurrentIndex(1);

}

void SCadMainWindow::registerToolAction(QAction* action, SToolMode tool_mode)
{
    action->setCheckable(true);
    m_tool_actions.emplace_back(static_cast<std::uint8_t>(tool_mode), action);
}

void SCadMainWindow::updateToolButtonHighlight(SToolMode current_mode)
{
    const std::uint8_t mode_byte = static_cast<std::uint8_t>(current_mode);
    for (const auto& entry : m_tool_actions)
    {
        entry.second->setChecked(entry.first == mode_byte);
    }
}

} // namespace vectorPath
