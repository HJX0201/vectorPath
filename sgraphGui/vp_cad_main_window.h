#pragma once

#include "vp_toolpath.h"

#include <SARibbonMainWindow.h>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

class QAction;
class QLabel;
class QCloseEvent;
class QAbstractButton;
class QToolBar;
class QToolButton;
class SARibbonCategory;
class SARibbonPanel;

namespace ads
{
class CDockManager;
class CDockWidget;
} // namespace ads

namespace Vp
{

class VpCadDocument;
class VpCadWorkspaceWidget;
class VpCommandLineWidget;
class VpDocumentRecoveryManager;
class VpShortcutManager;
class VpThemeManager;
class VpToolpathSimulationController;
class VpSelectionContextBar;
struct VpVectorImportGeometry;
enum class VpIconType;
enum class VpQuickEntityOperation;
enum class VpPolygonBooleanOperation : std::uint8_t;
enum class VpSvgLayerPriority : std::uint8_t;
enum class VpToolMode;

// 主窗口：组装 Ribbon、工作区与命令交互。
class VpCadMainWindow final : public SARibbonMainWindow
{
    Q_OBJECT

  public:
    // 构造主窗口并建立界面。
    explicit VpCadMainWindow(VpThemeManager& theme_manager, QWidget* parent = nullptr);
    // 析构。
    ~VpCadMainWindow() override;

  protected:
    // 关闭前保存工作区与文档。
    void closeEvent(QCloseEvent* event) override;

  private:
    // 创建全部命令动作。
    void createActions();
    // 创建 Ribbon 界面。
    void createRibbon();
    // 创建停靠工作区。
    void createDockingWorkspace();
    // 配置停靠窗口的分割比例。
    void configureDockSplitters();
    // 应用默认的工作区布局比例。
    void applyDefaultWorkspaceProportions();
    // 创建绘图工具条。
    QToolBar* createDrawingToolBar();
    // 创建图层面板的停靠窗口。
    ads::CDockWidget* createLayerDock();
    // 清空图层上的图元。
    bool clearLayerEntities(const QString& layer_name);
    // 创建状态栏控件。
    void createStatusBarWidgets();
    // 连接文档信号。
    void connectDocument();
    // 执行命令行输入的命令。
    void executeCommand(const QString& command);
    // 处理坐标输入形式的命令。
    bool executeCoordinateInput(const QString& command);
    // 预览命令输入的效果。
    void previewCommandInput(const QString& command);
    // 执行绘图辅助相关命令。
    bool executeDraftingCommand(const QString& normalized_command);
    // 执行夹点编辑命令。
    bool executeGripCommand(const QString& normalized_command);
    // 执行布尔运算命令。
    bool executeBooleanCommand(const QString& normalized_command);
    // 执行多边形布尔运算。
    void executePolygonBoolean(VpPolygonBooleanOperation operation);
    // 配置布尔运算面板。
    void configureBooleanPanel(SARibbonCategory* modify_category);
    // 配置夹点操作按钮。
    void configureGripAction(QAction* grip_action);
    // 显示单位设置对话框。
    void showUnitsDialog();
    // 刷新状态栏坐标显示。
    void updateCoordinateDisplay();
    // 执行阵列参数命令。
    bool executeArrayParameterCommand(const QString& normalized_command);
    // 执行视图相关命令。
    bool executeViewCommand(const QString& normalized_command);
    // 配置视图选项卡。
    void configureViewOptions(SARibbonCategory* view_category);
    // 执行图层相关命令。
    bool executeLayerCommand(const QString& simplified_command);
    // 执行输出相关命令。
    bool executeOutputCommand(const QString& simplified_command);
    // 执行快捷键相关命令。
    bool executeShortcutCommand(const QString& simplified_command);
    // 执行仿真相关命令。
    bool executeSimulationCommand(const QString& normalized_command);
    // 配置仿真面板。
    void configureSimulationPanel(SARibbonCategory* simulation_category);
    // 配置快捷操作面板。
    void configureQuickOperationPanel(SARibbonCategory* modify_category);
    // 配置快捷操作工具条。
    void configureQuickOperationBar();
    // 执行快捷图元操作。
    void executeQuickEntityOperation(VpQuickEntityOperation operation);
    // 选择全局绘图颜色。
    void chooseGlobalDrawingColor();
    // 修改选中图元的颜色。
    void chooseSelectedEntityColor();
    // 刷新全局颜色按钮的图标。
    void updateGlobalColorActionIcon();
    // 执行快捷操作命令。
    bool executeQuickOperationCommand(const QString& normalized_command);
    // 按选项对刀具路径排序。
    void sortToolpaths(const VpToolpathSortOptions& options);
    // 显示快捷键设置对话框。
    void showShortcutSettings();
    // 显示界面设置对话框。
    void showInterfaceSettings();
    // 应用界面缩放。
    void applyUiScale(int scale_percent);
    // 登记命令的快捷键。
    void registerShortcutAction(QAction* action, const QString& command_id,
                                const QKeySequence& default_sequence = {});
    // 显示打印样式管理器。
    void showPlotStyleManager();
    // 开始单行文字命令。
    void beginTextCommand();
    // 开始多行文字命令。
    void beginMTextCommand(const QString& initial_text = {});
    // 开始引线命令。
    void beginLeaderCommand(const QString& initial_text = {});
    // 配置注释面板。
    void configureAnnotationPanel(SARibbonPanel* annotation_panel);
    // 配置标注面板。
    void configureDimensionPanel(SARibbonPanel* annotation_panel);
    // 执行标注样式命令。
    bool executeDimensionStyleCommand(const QString& command);
    // 显示标注样式管理器。
    void showDimensionStyleManager();
    // 配置填充面板。
    void configureHatchPanel(SARibbonPanel* annotation_panel);
    // 执行填充命令。
    bool executeHatchCommand(const QString& command);
    // 显示填充设置对话框。
    void showHatchSettings(bool edit_selected);
    // 执行注释相关命令。
    bool executeAnnotationCommand(const QString& command);
    // 执行文字样式命令。
    bool executeTextStyleCommand(const QString& command);
    // 显示文字样式管理器。
    void showTextStyleManager();
    // 新建文档。
    void newDocument();
    // 打开文档。
    void openDocument();
    // 把 SVG 作为新文档打开。
    void openSvgAsNewDocument();
    // 把位图作为新文档打开。
    void openBitmapAsNewDocument();
    // 加载 SVG 为新文档。
    bool loadSvgAsNewDocument(const QString& file_path);
    // 加载位图为新文档。
    bool loadBitmapAsNewDocument(const QString& file_path);
    // 用矢量几何替换当前文档内容。
    bool replaceWithVectorGeometry(const VpVectorImportGeometry& geometry,
                                   const QString& source_description);
    // 填充导入的 SVG 色块。
    void fillImportedSvg();
    // 合并导入 SVG 中的重叠色块。
    void deduplicateImportedSvg(VpSvgLayerPriority priority);
    // 保存文档。
    bool saveDocument();
    // 文档另存为。
    bool saveDocumentAs();
    // 导出 DXF。
    void exportDxf();
    // 导出 DWG。
    void exportDwg();
    // 显示恢复管理器。
    void showRecoveryManager();
    // 生成一份恢复副本。
    void createRecoveryCopy();
    // 运行文档检查。
    void runDocumentAudit(bool repair);
    // 关闭前询问是否保存。
    bool maybeSave();
    // 刷新窗口标题。
    void updateWindowTitle();
    // 恢复上次的工作区布局。
    void restoreWorkspace();
    // 保存当前工作区布局。
    void saveWorkspace();
    // 按当前主题重建图标。
    void refreshIcons();
    // 创建菜单与工具条动作。
    QAction* createAction(const QString& text, VpIconType icon_type,
                          const QKeySequence& shortcut = {});
    // 创建状态栏按钮。
    QToolButton* createStatusButton(VpIconType icon_type, const QString& tool_tip, bool is_enabled,
                                    bool is_checked = false);

    VpThemeManager& m_theme_manager;
    std::unique_ptr<VpShortcutManager> m_shortcut_manager;
    std::unique_ptr<VpCadDocument> m_document;
    std::unique_ptr<VpDocumentRecoveryManager> m_recovery_manager;
    std::unique_ptr<VpToolpathSimulationController> m_simulation_controller;
    VpSelectionContextBar* m_selection_context_bar = nullptr;
    ads::CDockManager* m_dock_manager = nullptr;
    ads::CDockWidget* m_layer_dock = nullptr;
    ads::CDockWidget* m_property_dock = nullptr;
    ads::CDockWidget* m_command_dock = nullptr;
    VpCadWorkspaceWidget* m_workspace = nullptr;
    VpCommandLineWidget* m_command_line = nullptr;
    QLabel* m_coordinate_label = nullptr;
    double m_last_cursor_x = 0.0;
    double m_last_cursor_y = 0.0;
    QAbstractButton* m_application_button = nullptr;
    std::vector<std::pair<QAction*, VpIconType>> m_icon_actions;
    std::vector<std::pair<ads::CDockWidget*, VpIconType>> m_icon_docks;
    std::vector<std::pair<QToolButton*, VpIconType>> m_icon_status_buttons;

    QAction* m_new_action = nullptr;
    QAction* m_open_action = nullptr;
    QAction* m_save_action = nullptr;
    QAction* m_save_as_action = nullptr;
    QAction* m_import_svg_action = nullptr;
    QAction* m_import_bitmap_action = nullptr;
    QAction* m_svg_fill_action = nullptr;
    QAction* m_svg_deduplicate_action = nullptr;
    QAction* m_export_dxf_action = nullptr;
    QAction* m_export_dwg_action = nullptr;
    QAction* m_line_action = nullptr;
    QAction* m_circle_action = nullptr;
    QAction* m_arc_action = nullptr;
    QAction* m_ellipse_action = nullptr;
    QAction* m_spline_action = nullptr;
    QAction* m_undo_action = nullptr;
    QAction* m_redo_action = nullptr;
    QAction* m_zoom_extents_action = nullptr;
    QAction* m_snap_action = nullptr;
    QAction* m_grid_snap_action = nullptr;
    QAction* m_endpoint_snap_action = nullptr;
    QAction* m_center_snap_action = nullptr;
    QAction* m_node_display_action = nullptr;
    QAction* m_direction_display_action = nullptr;
    QAction* m_sequence_display_action = nullptr;
    QAction* m_theme_action = nullptr;
    QAction* m_dark_theme_action = nullptr;
    QAction* m_light_theme_action = nullptr;
    QAction* m_high_contrast_action = nullptr;
    QAction* m_global_color_action = nullptr;
    std::vector<QAction*> m_quick_operation_actions;
    VpToolpathSortOptions m_sort_options;
    int m_previous_tab_index = 1;
    // 登记工具按钮与工具模式的对应关系。
    void registerToolAction(QAction* action, VpToolMode tool_mode);
    // 更新当前工具的按钮高亮。
    void updateToolButtonHighlight(VpToolMode current_mode);
    std::vector<std::pair<std::uint8_t, QAction*>> m_tool_actions;
};

} // namespace Vp
