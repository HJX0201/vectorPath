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

class VpCadMainWindow final : public SARibbonMainWindow
{
    Q_OBJECT

  public:
    explicit VpCadMainWindow(VpThemeManager& theme_manager, QWidget* parent = nullptr);
    ~VpCadMainWindow() override;

  protected:
    void closeEvent(QCloseEvent* event) override;

  private:
    void createActions();
    void createRibbon();
    void createDockingWorkspace();
    void configureDockSplitters();
    void applyDefaultWorkspaceProportions();
    QToolBar* createDrawingToolBar();
    ads::CDockWidget* createLayerDock();
    bool clearLayerEntities(const QString& layer_name);
    void createStatusBarWidgets();
    void connectDocument();
    void executeCommand(const QString& command);
    bool executeCoordinateInput(const QString& command);
    void previewCommandInput(const QString& command);
    bool executeDraftingCommand(const QString& normalized_command);
    bool executeGripCommand(const QString& normalized_command);
    bool executeBooleanCommand(const QString& normalized_command);
    void executePolygonBoolean(VpPolygonBooleanOperation operation);
    void configureBooleanPanel(SARibbonCategory* modify_category);
    void configureGripAction(QAction* grip_action);
    void showUnitsDialog();
    void updateCoordinateDisplay();
    bool executeArrayParameterCommand(const QString& normalized_command);
    bool executeViewCommand(const QString& normalized_command);
    void configureViewOptions(SARibbonCategory* view_category);
    bool executeLayerCommand(const QString& simplified_command);
    bool executeOutputCommand(const QString& simplified_command);
    bool executeShortcutCommand(const QString& simplified_command);
    bool executeSimulationCommand(const QString& normalized_command);
    void configureSimulationPanel(SARibbonCategory* simulation_category);
    void configureQuickOperationPanel(SARibbonCategory* modify_category);
    void configureQuickOperationBar();
    void executeQuickEntityOperation(VpQuickEntityOperation operation);
    void chooseGlobalDrawingColor();
    void chooseSelectedEntityColor();
    void updateGlobalColorActionIcon();
    bool executeQuickOperationCommand(const QString& normalized_command);
    void sortToolpaths(const VpToolpathSortOptions& options);
    void showShortcutSettings();
    void showInterfaceSettings();
    void applyUiScale(int scale_percent);
    void registerShortcutAction(QAction* action, const QString& command_id,
                                const QKeySequence& default_sequence = {});
    void showPlotStyleManager();
    void beginTextCommand();
    void beginMTextCommand(const QString& initial_text = {});
    void beginLeaderCommand(const QString& initial_text = {});
    void configureAnnotationPanel(SARibbonPanel* annotation_panel);
    void configureDimensionPanel(SARibbonPanel* annotation_panel);
    bool executeDimensionStyleCommand(const QString& command);
    void showDimensionStyleManager();
    void configureHatchPanel(SARibbonPanel* annotation_panel);
    bool executeHatchCommand(const QString& command);
    void showHatchSettings(bool edit_selected);
    bool executeAnnotationCommand(const QString& command);
    bool executeTextStyleCommand(const QString& command);
    void showTextStyleManager();
    void newDocument();
    void openDocument();
    void openSvgAsNewDocument();
    void openBitmapAsNewDocument();
    bool loadSvgAsNewDocument(const QString& file_path);
    bool loadBitmapAsNewDocument(const QString& file_path);
    bool replaceWithVectorGeometry(const VpVectorImportGeometry& geometry,
                                   const QString& source_description);
    void fillImportedSvg();
    void deduplicateImportedSvg(VpSvgLayerPriority priority);
    bool saveDocument();
    bool saveDocumentAs();
    void exportDxf();
    void exportDwg();
    void showRecoveryManager();
    void createRecoveryCopy();
    void runDocumentAudit(bool repair);
    bool maybeSave();
    void updateWindowTitle();
    void restoreWorkspace();
    void saveWorkspace();
    void refreshIcons();
    QAction* createAction(const QString& text, VpIconType icon_type,
                          const QKeySequence& shortcut = {});
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
    void registerToolAction(QAction* action, VpToolMode tool_mode);
    void updateToolButtonHighlight(VpToolMode current_mode);
    std::vector<std::pair<std::uint8_t, QAction*>> m_tool_actions;
};

} // namespace Vp
