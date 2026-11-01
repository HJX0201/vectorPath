#pragma once

#include "vp_drafting_state.h"
#include "vp_entity.h"
#include "vp_geometry_types.h"
#include "vp_object_snap.h"
#include "vp_plot_style_table.h"
#include "vp_standard_shape.h"
#include "vp_toolpath.h"

#include <QColor>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QPointer>
#include <QRectF>
#include <QStringList>
#include <QVector>
#include <optional>
#include <vector>

class QMouseEvent;
class QPainter;
class QWheelEvent;

namespace Vp
{

class VpCadDocument;
struct VpCoordinateInput;

enum class VpGripRole
{
    MoveEntity,
    ControlPoint,
    RadiusPoint,
    ArcStartPoint,
    ArcEndPoint
};

enum class VpGripOperation
{
    Stretch,
    Move,
    Rotate,
    Scale,
    Mirror
};

// 视口中可拖拽的夹点：实体、角色与索引。
struct VpGripHandle
{
    VpEntityId entity_id = 0;
    VpGripRole role = VpGripRole::ControlPoint;
    std::size_t index = 0;
    VpPoint2d point;
};

// 计算图元的夹点集合。
std::vector<VpGripHandle> entityGripHandles(const VpEntityRecord& entity);
// 按夹点编辑得到新图元。
bool gripEditedEntity(const VpEntityRecord& source, const VpGripHandle& grip,
                      const VpPoint2d& destination, VpEntityRecord& result);
// 计算夹点编辑会影响的图元。
std::vector<VpEntityRecord> gripTransformedEntities(const std::vector<VpEntityRecord>& sources,
                                                    VpGripOperation operation,
                                                    const VpPoint2d& base_point,
                                                    const VpPoint2d& reference_point,
                                                    const VpPoint2d& destination);

enum class VpToolMode
{
    Select,
    Line,
    Circle,
    Polyline,
    PolylineEdit,
    Ellipse,
    Spline,
    SplineEdit,
    Rectangle,
    StandardShape,
    Arc,
    Move,
    Copy,
    Rotate,
    Scale,
    Mirror,
    Erase,
    Trim,
    Extend,
    Break,
    Join,
    Explode,
    Stretch,
    Lengthen,
    Fillet,
    Chamfer,
    Blend,
    Align,
    ArrayRect,
    ArrayPolar,
    ArrayPath,
    ArrayEdit,
    Offset,
    Text,
    MText,
    Leader,
    LinearDimension,
    AlignedDimension,
    AngularDimension,
    RadiusDimension,
    DiameterDimension,
    ArcLengthDimension,
    OrdinateDimension,
    Hatch
};

// 返回工具模式对应的标注类型。
std::optional<VpDimensionType> dimensionTypeForToolMode(VpToolMode tool_mode) noexcept;

enum class VpCircleConstruction
{
    CenterRadius,
    CenterDiameter,
    TwoPoint,
    ThreePoint,
    TangentTangentRadius,
    TangentTangentTangent
};

enum class VpArcConstruction
{
    ThreePoint,
    CenterStartEnd,
    StartCenterEnd,
    StartCenterAngle,
    CenterStartAngle,
    StartEndAngle,
    StartEndDirection,
    StartEndRadius
};

// OpenGL 视口：绘制、缩放、选择与夹点编辑。
class VpCadViewport final : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

  public:
    // 构造视口。
    explicit VpCadViewport(QWidget* parent = nullptr);

    // 绑定要显示的文档。
    void setDocument(VpCadDocument* document);
    // 切换当前工具。
    void setToolMode(VpToolMode tool_mode);
    // 返回当前工具模式。
    VpToolMode toolMode() const noexcept;
    // 取消正在进行的命令。
    void cancelCommand();
    // 缩放到全部图形。
    void zoomExtents();
    // 设置是否显示栅格。
    void setGridVisible(bool is_visible);
    // 返回栅格是否显示。
    bool isGridVisible() const noexcept;
    // 启用或关闭对象捕捉。
    void setObjectSnapEnabled(bool is_enabled);
    // 返回对象捕捉是否启用。
    bool isObjectSnapEnabled() const noexcept;
    // 启用或关闭端点捕捉。
    void setEndpointSnapEnabled(bool is_enabled);
    // 返回端点捕捉是否启用。
    bool isEndpointSnapEnabled() const noexcept;
    // 启用或关闭圆心捕捉。
    void setCenterSnapEnabled(bool is_enabled);
    // 返回圆心捕捉是否启用。
    bool isCenterSnapEnabled() const noexcept;
    // 启用或关闭正交模式。
    void setOrthoEnabled(bool is_enabled);
    // 返回正交模式是否启用。
    bool isOrthoEnabled() const noexcept;
    // 启用或关闭栅格捕捉。
    void setGridSnapEnabled(bool is_enabled);
    // 返回栅格捕捉是否启用。
    bool isGridSnapEnabled() const noexcept;
    // 设置栅格捕捉间距。
    void setGridSnapSpacing(double spacing);
    // 返回栅格捕捉间距。
    double gridSnapSpacing() const noexcept;
    // 设置栅格旋转角度。
    void setGridRotation(double rotation_degrees);
    // 返回栅格旋转角度。
    double gridRotation() const noexcept;
    // 启用或关闭极轴追踪。
    void setTrackingEnabled(bool is_enabled);
    // 返回极轴追踪是否启用。
    bool isTrackingEnabled() const noexcept;
    // 设置是否显示线宽。
    void setLineweightVisible(bool is_visible);
    // 返回线宽是否显示。
    bool isLineweightVisible() const noexcept;
    // 设置是否显示节点标记。
    void setNodeDisplayVisible(bool is_visible);
    // 返回节点标记是否显示。
    bool isNodeDisplayVisible() const noexcept;
    // 设置是否显示方向标记。
    void setDirectionDisplayVisible(bool is_visible);
    // 返回方向标记是否显示。
    bool isDirectionDisplayVisible() const noexcept;
    // 设置是否显示序号标记。
    void setSequenceDisplayVisible(bool is_visible);
    // 返回序号标记是否显示。
    bool isSequenceDisplayVisible() const noexcept;
    // 设置圆角半径。
    void setFilletRadius(double radius);
    // 返回圆角半径。
    double filletRadius() const noexcept;
    // 设置倒角距离。
    void setChamferDistances(double first_distance, double second_distance);
    // 设置矩形阵列的行列数。
    void setRectangularArrayCounts(int column_count, int row_count);
    // 设置环形阵列的项目数与填充角。
    void setPolarArrayParameters(int item_count, double fill_angle);
    // 设置路径阵列的项目数与是否对齐。
    void setPathArrayParameters(int item_count, bool align_to_path);
    // 修改选中矩形阵列的参数。
    bool editSelectedRectangularArray(int column_count, int row_count, double column_spacing,
                                      double row_spacing);
    // 修改选中环形阵列的参数。
    bool editSelectedPolarArray(int item_count, double fill_angle);
    // 修改选中路径阵列的参数。
    bool editSelectedPathArray(int item_count, bool align_to_path);
    // 设置圆的构造方式。
    void setCircleConstruction(VpCircleConstruction construction);
    // 设置相切圆的半径。
    void setCircleTangentRadius(double radius);
    // 设置圆弧构造方式。
    void setArcConstruction(VpArcConstruction construction);
    // 设置圆弧构造的参数。
    void setArcConstructionParameter(VpArcConstruction construction, double parameter);
    // 设置标准图形类型。
    void setStandardShapeType(VpStandardShapeType shape_type);
    // 返回标准图形类型。
    VpStandardShapeType standardShapeType() const noexcept;
    // 返回当前单选的图元标识。
    std::optional<VpEntityId> selectedEntityId() const noexcept;
    // 返回多选的图元标识集合。
    QVector<quint64> selectedEntityIds() const;
    // 设置选中的图元集合。
    void setSelectedEntityIds(const std::vector<VpEntityId>& entity_ids);
    // 返回选中图元集合的中心点。
    std::optional<VpPoint2d> entitySetBoundsCenter(const std::vector<VpEntityId>& entity_ids) const;
    // 全选。
    void selectAll();
    // 清空选择。
    void clearSelection();
    // 删除选中图元。
    void deleteSelected();
    // 进入夹点编辑。
    void beginGripEdit(VpGripOperation operation = VpGripOperation::Stretch);
    // 设置夹点操作类型。
    void setGripOperation(VpGripOperation operation);
    // 返回夹点操作类型。
    VpGripOperation gripOperation() const noexcept;
    // 循环切换夹点操作。
    void cycleGripOperation();
    // 用窗口选择图元。
    void selectEntitiesInWindow(const VpPoint2d& first_corner, const VpPoint2d& second_corner,
                                bool is_crossing, bool is_additive = false);
    // 设置待放置的单行文字。
    void setPendingText(QString text);
    // 设置待放置文字的对齐方式。
    void setPendingTextAlignment(
        VpTextHorizontalAlignment alignment,
        VpTextVerticalAlignment vertical_alignment = VpTextVerticalAlignment::Baseline);
    // 设置待放置的多行文字。
    void setPendingMText(QString rich_text, double width, double height,
                         VpTextHorizontalAlignment alignment);
    // 设置待放置的引线。
    void setPendingLeader(QString text, double text_height, double arrow_size);
    // 设置填充参数。
    void setHatchSettings(const VpHatchEntity& hatch_settings);
    // 返回当前填充参数。
    VpHatchEntity hatchSettings() const;
    // 修改选中填充的参数。
    bool editSelectedHatches(const VpHatchEntity& hatch_settings);
    // 提交一个世界坐标点。
    void submitWorldPoint(const VpPoint2d& world_point);
    // 提交坐标输入。
    bool submitCoordinateInput(const VpCoordinateInput& input);
    // 提交命令关键字。
    bool submitCommandKeyword(const QString& keyword);
    // 预览坐标输入结果。
    bool previewCoordinateInput(const VpCoordinateInput& input);
    // 清除坐标输入预览。
    void clearCoordinateInputPreview();
    // 返回命令预览点。
    std::optional<VpPoint2d> commandPreviewPoint() const noexcept;
    // 设置画布与栅格配色。
    void setAppearance(const QColor& canvas_color, const QColor& grid_color,
                       const QColor& major_grid_color, const QColor& text_color);
    // 返回可见区域左下角的世界坐标。
    VpPoint2d visibleWorldBottomLeft() const;
    // 设置仿真叠加层。
    void setSimulationOverlay(std::vector<VpToolpathMotion> motions,
                              std::size_t completed_motion_count, bool trace_visible);
    // 清除仿真叠加层。
    void clearSimulationOverlay();
  signals:
    // 光标位置变化时发出。
    void cursorWorldPositionChanged(const Vp::VpPoint2d& world_position);
    // 命令消息发出。
    void commandMessage(const QString& message);
    // 工具模式变化时发出。
    void toolModeChanged(Vp::VpToolMode tool_mode);
    // 单选图元变化时发出。
    void selectedEntityChanged(quint64 entity_id);
    // 选择集合变化时发出。
    void selectionChanged(const QVector<quint64>& entity_ids);
    // 请求弹出选择操作条时发出。
    void selectionContextRequested(const QPoint& global_position);
    // 栅格可见性变化时发出。
    void gridVisibilityChanged(bool is_visible);
    // 对象捕捉开关变化时发出。
    void objectSnapChanged(bool is_enabled);
    // 捕捉模式变化时发出。
    void objectSnapModesChanged(bool endpoint_enabled, bool center_enabled);
    // 正交开关变化时发出。
    void orthoChanged(bool is_enabled);
    // 栅格捕捉开关变化时发出。
    void gridSnapChanged(bool is_enabled);
    // 夹点操作变化时发出。
    void gripOperationChanged(Vp::VpGripOperation operation);
    // 极轴追踪开关变化时发出。
    void trackingChanged(bool is_enabled);
    // 图元显示选项变化时发出。
    void entityDisplayOptionsChanged(bool nodes_visible, bool directions_visible,
                                     bool sequence_visible);

  protected:
    // 初始化 OpenGL 上下文。
    void initializeGL() override;
    // 绘制一帧。
    void paintGL() override;
    // 视口尺寸变化时调整投影。
    void resizeGL(int width, int height) override;
    // 处理鼠标按下。
    void mousePressEvent(QMouseEvent* event) override;
    // 处理鼠标移动。
    void mouseMoveEvent(QMouseEvent* event) override;
    // 处理鼠标松开。
    void mouseReleaseEvent(QMouseEvent* event) override;
    // 处理滚轮缩放。
    void wheelEvent(QWheelEvent* event) override;
    // 处理键盘输入。
    void keyPressEvent(QKeyEvent* event) override;

  private:
    // 世界坐标转屏幕坐标。
    QPointF worldToScreen(const VpPoint2d& world_point) const;
    // 屏幕坐标转世界坐标。
    VpPoint2d screenToWorld(const QPointF& screen_point) const;
    // 绘制栅格。
    void drawGrid(QPainter& painter);
    // 绘制图元。
    void drawEntities(QPainter& painter);
    // 绘制指定空间中的图元。
    void drawEntitiesForSpace(QPainter& painter, bool show_selection, bool plottable_only = false);
    // 绘制单个图元的几何。
    void drawEntityGeometry(QPainter& painter, const VpEntityRecord& entity,
                            const QColor& fill_color);
    // 绘制命令预览。
    void drawPreview(QPainter& painter);
    // 绘制对象捕捉标记。
    void drawObjectSnapMarker(QPainter& painter);
    // 绘制裁剪预览。
    void drawTrimPreview(QPainter& painter);
    // 绘制延伸预览。
    void drawExtendPreview(QPainter& painter);
    // 绘制打断预览。
    void drawBreakPreview(QPainter& painter);
    // 绘制合并预览。
    void drawJoinPreview(QPainter& painter);
    // 绘制分解预览。
    void drawExplodePreview(QPainter& painter);
    // 绘制拉伸预览。
    void drawStretchPreview(QPainter& painter);
    // 绘制加长预览。
    void drawLengthenPreview(QPainter& painter);
    // 绘制圆角预览。
    void drawFilletPreview(QPainter& painter);
    // 绘制倒角预览。
    void drawChamferPreview(QPainter& painter);
    // 绘制样条桥接预览。
    void drawBlendPreview(QPainter& painter);
    // 绘制对齐预览。
    void drawAlignPreview(QPainter& painter);
    // 绘制矩形阵列预览。
    void drawRectangularArrayPreview(QPainter& painter);
    // 绘制环形阵列预览。
    void drawPolarArrayPreview(QPainter& painter);
    // 绘制路径阵列预览。
    void drawPathArrayPreview(QPainter& painter);
    // 绘制阵列编辑预览。
    void drawArrayEditPreview(QPainter& painter);
    // 绘制样条编辑预览。
    void drawSplineEditPreview(QPainter& painter);
    // 绘制注释预览。
    void drawAnnotationPreview(QPainter& painter);
    // 绘制标注预览。
    void drawDimensionPreview(QPainter& painter);
    // 绘制标注图元。
    void drawDimensionEntity(QPainter& painter, const VpLinearDimensionEntity& dimension);
    // 绘制填充图元。
    void drawHatchEntity(QPainter& painter, const VpHatchEntity& hatch, const QColor& fill_color);
    // 绘制圆的构造预览。
    void drawCircleConstructionPreview(QPainter& painter);
    // 绘制圆弧构造预览。
    void drawArcConstructionPreview(QPainter& painter);
    // 绘制导航控件叠加层。
    void drawNavigationOverlay(QPainter& painter);
    // 绘制仿真叠加层。
    void drawSimulationOverlay(QPainter& painter);
    // 绘制图元显示标记叠加层。
    void drawEntityDisplayOverlay(QPainter& painter);
    // 绘制夹点。
    void drawGrips(QPainter& painter);
    // 接受一个拾取点。
    void acceptPoint(const VpPoint2d& world_point);
    // 对拾取点应用捕捉与约束。
    VpPoint2d constrainedPoint(const VpPoint2d& world_point);
    // 返回命中点处的图元。
    std::optional<VpEntityId> entityAt(const VpPoint2d& world_point) const;
    // 按标识查找图元。
    const VpEntityRecord* entityById(VpEntityId entity_id) const;
    // 在拾取点处选择图元。
    void selectAt(const VpPoint2d& world_point);
    // 按两个角点选择图元。
    void selectInWindow(const VpPoint2d& first_corner, const VpPoint2d& second_corner,
                        bool is_crossing);
    // 返回图元的包围盒。
    QRectF entityBounds(const VpEntityRecord& entity) const;
    // 广播选择状态。
    void emitSelectionState();
    // 完成移动。
    void completeMove(const VpPoint2d& destination);
    // 完成复制。
    void completeCopy(const VpPoint2d& destination);
    // 完成旋转。
    void completeRotate(const VpPoint2d& destination);
    // 完成缩放。
    void completeScale(const VpPoint2d& destination);
    // 完成镜像。
    void completeMirror(const VpPoint2d& axis_end);
    // 完成拉伸。
    void completeStretch(const VpPoint2d& destination);
    // 接受加长的拾取点。
    void acceptLengthenPoint(const VpPoint2d& world_point);
    // 接受圆角的拾取点。
    void acceptFilletPoint(const VpPoint2d& world_point);
    // 接受倒角的拾取点。
    void acceptChamferPoint(const VpPoint2d& world_point);
    // 接受桥接的拾取点。
    void acceptBlendPoint(const VpPoint2d& world_point);
    // 接受对齐的拾取点。
    void acceptAlignPoint(const VpPoint2d& world_point);
    // 接受矩形阵列的拾取点。
    void acceptRectangularArrayPoint(const VpPoint2d& world_point);
    // 接受环形阵列的拾取点。
    void acceptPolarArrayPoint(const VpPoint2d& world_point);
    // 接受路径阵列的拾取点。
    void acceptPathArrayPoint(const VpPoint2d& world_point);
    // 接受阵列编辑的拾取点。
    void acceptArrayEditPoint(const VpPoint2d& world_point);
    // 修改选中关联阵列的参数。
    bool editAssociativeArray(VpAssociativeArrayData parameters);
    // 接受椭圆的拾取点。
    void acceptEllipsePoint(const VpPoint2d& world_point);
    // 接受样条的拾取点。
    void acceptSplinePoint(const VpPoint2d& world_point);
    // 接受多行文字的拾取点。
    void acceptMTextPoint(const VpPoint2d& world_point);
    // 接受引线的拾取点。
    void acceptLeaderPoint(const VpPoint2d& world_point);
    // 接受标注的拾取点。
    void acceptDimensionPoint(const VpPoint2d& world_point);
    // 由边界图元生成填充数据。
    std::optional<VpHatchEntity> hatchFromBoundaryEntity(const VpEntityRecord& boundary) const;
    // 接受样条编辑的拾取点。
    void acceptSplineEditPoint(const VpPoint2d& world_point);
    // 处理样条编辑命令的关键字。
    bool handleSplineEditKeyword(const QString& normalized_keyword);
    // 显示样条编辑提示。
    void showSplineEditPrompt();
    // 接受圆的构造拾取点。
    void acceptCircleConstructionPoint(const VpPoint2d& world_point);
    // 接受相切圆的拾取点。
    void acceptTangentCirclePoint(const VpPoint2d& world_point);
    // 计算相切圆的预览。
    bool tangentCirclePreview(const VpPoint2d& candidate_pick, VpCircleEntity& result) const;
    // 接受圆弧构造的拾取点。
    void acceptArcConstructionPoint(const VpPoint2d& world_point);
    // 接受标准图形的拾取点。
    void acceptStandardShapePoint(const VpPoint2d& world_point);
    // 完成圆弧构造。
    void completeArcConstruction();
    // 接受多段线编辑的拾取点。
    void acceptPolylineEditPoint(const VpPoint2d& world_point);
    // 处理多段线编辑命令的关键字。
    bool handlePolylineEditKeyword(const QString& normalized_keyword);
    // 显示多段线编辑提示。
    void showPolylineEditPrompt();
    // 完成多段线的合并。
    void completePolylineEditJoin();
    // 完成偏移。
    void completeOffset(const VpPoint2d& through_point);
    // 完成合并。
    void completeJoin();
    // 完成多段线绘制。
    void completePolyline(bool is_closed = false);
    // 开始夹点拖拽。
    bool beginGripDrag(const QPointF& screen_point);
    // 更新夹点拖拽位置。
    void updateGripDrag(const VpPoint2d& destination);
    // 提交夹点拖拽。
    void commitGripDrag();
    // 取消夹点拖拽。
    void cancelGripDrag();
    // 计算极轴捕捉结果。
    std::optional<VpObjectSnapResult> trackingSnap(const VpPoint2d& cursor,
                                                   const std::optional<VpPoint2d>& reference_point,
                                                   double tolerance) const;
    // 返回自适应栅格间距。
    double adaptiveGridSpacing() const;
    // 返回坐标输入的参考点。
    std::optional<VpPoint2d> coordinateReferencePoint() const noexcept;

    QPointer<VpCadDocument> m_document;
    VpToolMode m_tool_mode = VpToolMode::Select;
    VpToolMode m_last_tool_mode = VpToolMode::Line;
    std::optional<VpPoint2d> m_first_point;
    std::optional<VpPoint2d> m_command_start_point;
    std::vector<VpPoint2d> m_input_points;
    std::vector<double> m_polyline_bulges;
    std::vector<double> m_polyline_start_widths;
    std::vector<double> m_polyline_end_widths;
    std::optional<VpPoint2d> m_polyline_arc_point;
    std::optional<std::size_t> m_spline_edit_control_index;
    std::optional<VpEntityId> m_selected_entity_id;
    std::vector<VpEntityId> m_selected_entity_ids;
    std::vector<VpEntityId> m_curve_reference_ids;
    std::optional<VpEntityId> m_reference_entity_id;
    std::optional<VpObjectSnapResult> m_active_object_snap;
    VpDraftingState m_drafting_state;
    std::optional<VpGripHandle> m_active_grip;
    std::vector<VpEntityRecord> m_grip_sources;
    std::vector<VpEntityRecord> m_grip_previews;
    VpPoint2d m_grip_reference_point;
    VpPoint2d m_cursor_world;
    VpPoint2d m_pointer_world;
    std::optional<VpPoint2d> m_command_preview_point;
    QPoint m_last_mouse_position;
    QPoint m_selection_start_screen;
    QPoint m_selection_current_screen;
    VpPoint2d m_selection_start_world;
    QPointF m_pan_offset;
    double m_zoom = 1.0;
    double m_fillet_radius = 5.0;
    double m_chamfer_first_distance = 5.0;
    double m_chamfer_second_distance = 5.0;
    double m_polyline_start_width = 0.0;
    double m_polyline_end_width = 0.0;
    double m_circle_tangent_radius = 5.0;
    int m_array_column_count = 3;
    int m_array_row_count = 2;
    int m_array_polar_item_count = 6;
    double m_array_polar_fill_angle = 360.0;
    int m_array_path_item_count = 6;
    int m_line_segment_count = 0;
    bool m_array_path_align = true;
    bool m_align_scale_enabled = true;
    bool m_polyline_arc_mode = false;
    bool m_is_panning = false;
    bool m_is_box_selecting = false;
    bool m_is_grip_dragging = false;
    bool m_grip_has_change = false;
    VpGripOperation m_grip_operation = VpGripOperation::Stretch;
    VpCircleConstruction m_circle_construction = VpCircleConstruction::CenterRadius;
    VpArcConstruction m_arc_construction = VpArcConstruction::ThreePoint;
    VpStandardShapeType m_standard_shape_type = VpStandardShapeType::FivePointStar;
    std::optional<double> m_arc_construction_parameter;
    bool m_is_additive_selection = false;
    bool m_is_grid_visible = true;
    bool m_is_object_snap_enabled = true;
    VpObjectSnapModes m_object_snap_modes = objectSnapModeValue(VpObjectSnapMode::Endpoint) |
                                            objectSnapModeValue(VpObjectSnapMode::Center);
    bool m_is_lineweight_visible = false;
    bool m_is_node_display_visible = false;
    bool m_is_direction_display_visible = false;
    bool m_is_sequence_display_visible = false;
    QColor m_canvas_color{8, 11, 15};
    QColor m_grid_color{31, 40, 51};
    QColor m_major_grid_color{58, 72, 88};
    QColor m_overlay_text_color{183, 192, 202};
    QString m_plot_style_override;
    std::optional<VpPlotStyleTable> m_plot_style_table_override;
    QString m_pending_text;
    VpTextHorizontalAlignment m_pending_text_alignment = VpTextHorizontalAlignment::Left;
    VpTextVerticalAlignment m_pending_text_vertical_alignment = VpTextVerticalAlignment::Baseline;
    VpMTextEntity m_pending_mtext;
    QString m_pending_leader_text;
    double m_pending_leader_text_height = 2.5;
    double m_pending_leader_arrow_size = 2.5;
    VpHatchEntity m_hatch_settings;
    std::vector<VpToolpathMotion> m_simulation_motions;
    std::size_t m_completed_simulation_motion_count = 0;
    bool m_is_simulation_overlay_active = false;
    bool m_is_simulation_trace_visible = true;
};

} // namespace Vp
