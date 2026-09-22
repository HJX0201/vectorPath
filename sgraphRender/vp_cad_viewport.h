#pragma once

#include "vp_cad_viewport_drafting.h"
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

struct VpGripHandle
{
    VpEntityId entity_id = 0;
    VpGripRole role = VpGripRole::ControlPoint;
    std::size_t index = 0;
    VpPoint2d point;
};

std::vector<VpGripHandle> entityGripHandles(const VpEntityRecord& entity);
bool gripEditedEntity(const VpEntityRecord& source, const VpGripHandle& grip,
                      const VpPoint2d& destination, VpEntityRecord& result);
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

class VpCadViewport final : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

  public:
    explicit VpCadViewport(QWidget* parent = nullptr);

    void setDocument(VpCadDocument* document);
    void setToolMode(VpToolMode tool_mode);
    VpToolMode toolMode() const noexcept;
    void cancelCommand();
    void zoomExtents();
    void setGridVisible(bool is_visible);
    bool isGridVisible() const noexcept;
    void setObjectSnapEnabled(bool is_enabled);
    bool isObjectSnapEnabled() const noexcept;
    void setEndpointSnapEnabled(bool is_enabled);
    bool isEndpointSnapEnabled() const noexcept;
    void setCenterSnapEnabled(bool is_enabled);
    bool isCenterSnapEnabled() const noexcept;
    void setOrthoEnabled(bool is_enabled);
    bool isOrthoEnabled() const noexcept;
    void setGridSnapEnabled(bool is_enabled);
    bool isGridSnapEnabled() const noexcept;
    void setGridSnapSpacing(double spacing);
    double gridSnapSpacing() const noexcept;
    void setGridRotation(double rotation_degrees);
    double gridRotation() const noexcept;
    void setTrackingEnabled(bool is_enabled);
    bool isTrackingEnabled() const noexcept;
    void setLineweightVisible(bool is_visible);
    bool isLineweightVisible() const noexcept;
    void setNodeDisplayVisible(bool is_visible);
    bool isNodeDisplayVisible() const noexcept;
    void setDirectionDisplayVisible(bool is_visible);
    bool isDirectionDisplayVisible() const noexcept;
    void setSequenceDisplayVisible(bool is_visible);
    bool isSequenceDisplayVisible() const noexcept;
    void setFilletRadius(double radius);
    double filletRadius() const noexcept;
    void setChamferDistances(double first_distance, double second_distance);
    void setRectangularArrayCounts(int column_count, int row_count);
    void setPolarArrayParameters(int item_count, double fill_angle);
    void setPathArrayParameters(int item_count, bool align_to_path);
    bool editSelectedRectangularArray(int column_count, int row_count, double column_spacing,
                                      double row_spacing);
    bool editSelectedPolarArray(int item_count, double fill_angle);
    bool editSelectedPathArray(int item_count, bool align_to_path);
    void setCircleConstruction(VpCircleConstruction construction);
    void setCircleTangentRadius(double radius);
    void setArcConstruction(VpArcConstruction construction);
    void setArcConstructionParameter(VpArcConstruction construction, double parameter);
    void setStandardShapeType(VpStandardShapeType shape_type);
    VpStandardShapeType standardShapeType() const noexcept;
    std::optional<VpEntityId> selectedEntityId() const noexcept;
    QVector<quint64> selectedEntityIds() const;
    void setSelectedEntityIds(const std::vector<VpEntityId>& entity_ids);
    std::optional<VpPoint2d> entitySetBoundsCenter(const std::vector<VpEntityId>& entity_ids) const;
    void selectAll();
    void clearSelection();
    void deleteSelected();
    void beginGripEdit(VpGripOperation operation = VpGripOperation::Stretch);
    void setGripOperation(VpGripOperation operation);
    VpGripOperation gripOperation() const noexcept;
    void cycleGripOperation();
    void selectEntitiesInWindow(const VpPoint2d& first_corner, const VpPoint2d& second_corner,
                                bool is_crossing, bool is_additive = false);
    void setPendingText(QString text);
    void setPendingTextAlignment(
        VpTextHorizontalAlignment alignment,
        VpTextVerticalAlignment vertical_alignment = VpTextVerticalAlignment::Baseline);
    void setPendingMText(QString rich_text, double width, double height,
                         VpTextHorizontalAlignment alignment);
    void setPendingLeader(QString text, double text_height, double arrow_size);
    void setHatchSettings(const VpHatchEntity& hatch_settings);
    VpHatchEntity hatchSettings() const;
    bool editSelectedHatches(const VpHatchEntity& hatch_settings);
    void submitWorldPoint(const VpPoint2d& world_point);
    bool submitCoordinateInput(const VpCoordinateInput& input);
    bool submitCommandKeyword(const QString& keyword);
    bool previewCoordinateInput(const VpCoordinateInput& input);
    void clearCoordinateInputPreview();
    std::optional<VpPoint2d> commandPreviewPoint() const noexcept;
    void setAppearance(const QColor& canvas_color, const QColor& grid_color,
                       const QColor& major_grid_color, const QColor& text_color);
    VpPoint2d visibleWorldBottomLeft() const;
    void setSimulationOverlay(std::vector<VpToolpathMotion> motions,
                              std::size_t completed_motion_count, bool trace_visible);
    void clearSimulationOverlay();
  signals:
    void cursorWorldPositionChanged(const Vp::VpPoint2d& world_position);
    void commandMessage(const QString& message);
    void toolModeChanged(Vp::VpToolMode tool_mode);
    void selectedEntityChanged(quint64 entity_id);
    void selectionChanged(const QVector<quint64>& entity_ids);
    void selectionContextRequested(const QPoint& global_position);
    void gridVisibilityChanged(bool is_visible);
    void objectSnapChanged(bool is_enabled);
    void objectSnapModesChanged(bool endpoint_enabled, bool center_enabled);
    void orthoChanged(bool is_enabled);
    void gridSnapChanged(bool is_enabled);
    void gripOperationChanged(Vp::VpGripOperation operation);
    void trackingChanged(bool is_enabled);
    void entityDisplayOptionsChanged(bool nodes_visible, bool directions_visible,
                                     bool sequence_visible);

  protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

  private:
    QPointF worldToScreen(const VpPoint2d& world_point) const;
    VpPoint2d screenToWorld(const QPointF& screen_point) const;
    void drawGrid(QPainter& painter);
    void drawEntities(QPainter& painter);
    void drawEntitiesForSpace(QPainter& painter, bool show_selection, bool plottable_only = false);
    void drawEntityGeometry(QPainter& painter, const VpEntityRecord& entity,
                            const QColor& fill_color);
    void drawPreview(QPainter& painter);
    void drawObjectSnapMarker(QPainter& painter);
    void drawTrimPreview(QPainter& painter);
    void drawExtendPreview(QPainter& painter);
    void drawBreakPreview(QPainter& painter);
    void drawJoinPreview(QPainter& painter);
    void drawExplodePreview(QPainter& painter);
    void drawStretchPreview(QPainter& painter);
    void drawLengthenPreview(QPainter& painter);
    void drawFilletPreview(QPainter& painter);
    void drawChamferPreview(QPainter& painter);
    void drawBlendPreview(QPainter& painter);
    void drawAlignPreview(QPainter& painter);
    void drawRectangularArrayPreview(QPainter& painter);
    void drawPolarArrayPreview(QPainter& painter);
    void drawPathArrayPreview(QPainter& painter);
    void drawArrayEditPreview(QPainter& painter);
    void drawSplineEditPreview(QPainter& painter);
    void drawAnnotationPreview(QPainter& painter);
    void drawDimensionPreview(QPainter& painter);
    void drawDimensionEntity(QPainter& painter, const VpLinearDimensionEntity& dimension);
    void drawHatchEntity(QPainter& painter, const VpHatchEntity& hatch, const QColor& fill_color);
    void drawCircleConstructionPreview(QPainter& painter);
    void drawArcConstructionPreview(QPainter& painter);
    void drawNavigationOverlay(QPainter& painter);
    void drawSimulationOverlay(QPainter& painter);
    void drawEntityDisplayOverlay(QPainter& painter);
    void drawGrips(QPainter& painter);
    void acceptPoint(const VpPoint2d& world_point);
    VpPoint2d constrainedPoint(const VpPoint2d& world_point);
    std::optional<VpEntityId> entityAt(const VpPoint2d& world_point) const;
    const VpEntityRecord* entityById(VpEntityId entity_id) const;
    void selectAt(const VpPoint2d& world_point);
    void selectInWindow(const VpPoint2d& first_corner, const VpPoint2d& second_corner,
                        bool is_crossing);
    QRectF entityBounds(const VpEntityRecord& entity) const;
    void emitSelectionState();
    void completeMove(const VpPoint2d& destination);
    void completeCopy(const VpPoint2d& destination);
    void completeRotate(const VpPoint2d& destination);
    void completeScale(const VpPoint2d& destination);
    void completeMirror(const VpPoint2d& axis_end);
    void completeStretch(const VpPoint2d& destination);
    void acceptLengthenPoint(const VpPoint2d& world_point);
    void acceptFilletPoint(const VpPoint2d& world_point);
    void acceptChamferPoint(const VpPoint2d& world_point);
    void acceptBlendPoint(const VpPoint2d& world_point);
    void acceptAlignPoint(const VpPoint2d& world_point);
    void acceptRectangularArrayPoint(const VpPoint2d& world_point);
    void acceptPolarArrayPoint(const VpPoint2d& world_point);
    void acceptPathArrayPoint(const VpPoint2d& world_point);
    void acceptArrayEditPoint(const VpPoint2d& world_point);
    bool editAssociativeArray(VpAssociativeArrayData parameters);
    void acceptEllipsePoint(const VpPoint2d& world_point);
    void acceptSplinePoint(const VpPoint2d& world_point);
    void acceptMTextPoint(const VpPoint2d& world_point);
    void acceptLeaderPoint(const VpPoint2d& world_point);
    void acceptDimensionPoint(const VpPoint2d& world_point);
    std::optional<VpHatchEntity> hatchFromBoundaryEntity(const VpEntityRecord& boundary) const;
    void acceptSplineEditPoint(const VpPoint2d& world_point);
    bool handleSplineEditKeyword(const QString& normalized_keyword);
    void showSplineEditPrompt();
    void acceptCircleConstructionPoint(const VpPoint2d& world_point);
    void acceptTangentCirclePoint(const VpPoint2d& world_point);
    bool tangentCirclePreview(const VpPoint2d& candidate_pick, VpCircleEntity& result) const;
    void acceptArcConstructionPoint(const VpPoint2d& world_point);
    void acceptStandardShapePoint(const VpPoint2d& world_point);
    void completeArcConstruction();
    void acceptPolylineEditPoint(const VpPoint2d& world_point);
    bool handlePolylineEditKeyword(const QString& normalized_keyword);
    void showPolylineEditPrompt();
    void completePolylineEditJoin();
    void completeOffset(const VpPoint2d& through_point);
    void completeJoin();
    void completePolyline(bool is_closed = false);
    bool beginGripDrag(const QPointF& screen_point);
    void updateGripDrag(const VpPoint2d& destination);
    void commitGripDrag();
    void cancelGripDrag();
    std::optional<VpObjectSnapResult> trackingSnap(const VpPoint2d& cursor,
                                                   const std::optional<VpPoint2d>& reference_point,
                                                   double tolerance) const;
    double adaptiveGridSpacing() const;
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
