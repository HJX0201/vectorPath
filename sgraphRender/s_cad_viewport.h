#pragma once

#include "s_cad_viewport_drafting.h"
#include "s_entity.h"
#include "s_geometry_types.h"
#include "s_object_snap.h"
#include "s_plot_style_table.h"
#include "s_standard_shape.h"
#include "s_toolpath.h"

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

namespace vectorPath
{

class SCadDocument;
struct SCoordinateInput;

enum class SGripRole
{
    MoveEntity,
    ControlPoint,
    RadiusPoint,
    ArcStartPoint,
    ArcEndPoint
};

enum class SGripOperation
{
    Stretch,
    Move,
    Rotate,
    Scale,
    Mirror
};

struct SGripHandle
{
    SEntityId entity_id = 0;
    SGripRole role = SGripRole::ControlPoint;
    std::size_t index = 0;
    SPoint2d point;
};

std::vector<SGripHandle> entityGripHandles(const SEntityRecord& entity);
bool gripEditedEntity(const SEntityRecord& source, const SGripHandle& grip,
                      const SPoint2d& destination, SEntityRecord& result);
std::vector<SEntityRecord> gripTransformedEntities(const std::vector<SEntityRecord>& sources,
                                                   SGripOperation operation,
                                                   const SPoint2d& base_point,
                                                   const SPoint2d& reference_point,
                                                   const SPoint2d& destination);

enum class SToolMode
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

std::optional<SDimensionType> dimensionTypeForToolMode(SToolMode tool_mode) noexcept;

enum class SCircleConstruction
{
    CenterRadius,
    CenterDiameter,
    TwoPoint,
    ThreePoint,
    TangentTangentRadius,
    TangentTangentTangent
};

enum class SArcConstruction
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

class SCadViewport final : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

  public:
    explicit SCadViewport(QWidget* parent = nullptr);

    void setDocument(SCadDocument* document);
    void setToolMode(SToolMode tool_mode);
    SToolMode toolMode() const noexcept;
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
    void setCircleConstruction(SCircleConstruction construction);
    void setCircleTangentRadius(double radius);
    void setArcConstruction(SArcConstruction construction);
    void setArcConstructionParameter(SArcConstruction construction, double parameter);
    void setStandardShapeType(SStandardShapeType shape_type);
    SStandardShapeType standardShapeType() const noexcept;
    std::optional<SEntityId> selectedEntityId() const noexcept;
    QVector<quint64> selectedEntityIds() const;
    void setSelectedEntityIds(const std::vector<SEntityId>& entity_ids);
    std::optional<SPoint2d> entitySetBoundsCenter(
        const std::vector<SEntityId>& entity_ids) const;
    void selectAll();
    void clearSelection();
    void deleteSelected();
    void beginGripEdit(SGripOperation operation = SGripOperation::Stretch);
    void setGripOperation(SGripOperation operation);
    SGripOperation gripOperation() const noexcept;
    void cycleGripOperation();
    void selectEntitiesInWindow(const SPoint2d& first_corner, const SPoint2d& second_corner,
                                bool is_crossing, bool is_additive = false);
    void setPendingText(QString text);
    void setPendingTextAlignment(
        STextHorizontalAlignment alignment,
        STextVerticalAlignment vertical_alignment = STextVerticalAlignment::Baseline);
    void setPendingMText(QString rich_text, double width, double height,
                         STextHorizontalAlignment alignment);
    void setPendingLeader(QString text, double text_height, double arrow_size);
    void setHatchSettings(const SHatchEntity& hatch_settings);
    SHatchEntity hatchSettings() const;
    bool editSelectedHatches(const SHatchEntity& hatch_settings);
    void submitWorldPoint(const SPoint2d& world_point);
    bool submitCoordinateInput(const SCoordinateInput& input);
    bool submitCommandKeyword(const QString& keyword);
    bool previewCoordinateInput(const SCoordinateInput& input);
    void clearCoordinateInputPreview();
    std::optional<SPoint2d> commandPreviewPoint() const noexcept;
    void setAppearance(const QColor& canvas_color, const QColor& grid_color,
                       const QColor& major_grid_color, const QColor& text_color);
    SPoint2d visibleWorldBottomLeft() const;
    void setSimulationOverlay(std::vector<SToolpathMotion> motions,
                              std::size_t completed_motion_count, bool trace_visible);
    void clearSimulationOverlay();
  signals:
    void cursorWorldPositionChanged(const vectorPath::SPoint2d& world_position);
    void commandMessage(const QString& message);
    void toolModeChanged(vectorPath::SToolMode tool_mode);
    void selectedEntityChanged(quint64 entity_id);
    void selectionChanged(const QVector<quint64>& entity_ids);
    void selectionContextRequested(const QPoint& global_position);
    void gridVisibilityChanged(bool is_visible);
    void objectSnapChanged(bool is_enabled);
    void objectSnapModesChanged(bool endpoint_enabled, bool center_enabled);
    void orthoChanged(bool is_enabled);
    void gridSnapChanged(bool is_enabled);
    void gripOperationChanged(vectorPath::SGripOperation operation);
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
    QPointF worldToScreen(const SPoint2d& world_point) const;
    SPoint2d screenToWorld(const QPointF& screen_point) const;
    void drawGrid(QPainter& painter);
    void drawEntities(QPainter& painter);
    void drawEntitiesForSpace(QPainter& painter, bool show_selection,
                              bool plottable_only = false);
    void drawEntityGeometry(QPainter& painter, const SEntityRecord& entity,
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
    void drawDimensionEntity(QPainter& painter, const SLinearDimensionEntity& dimension);
    void drawHatchEntity(QPainter& painter, const SHatchEntity& hatch, const QColor& fill_color);
    void drawCircleConstructionPreview(QPainter& painter);
    void drawArcConstructionPreview(QPainter& painter);
    void drawNavigationOverlay(QPainter& painter);
    void drawSimulationOverlay(QPainter& painter);
    void drawEntityDisplayOverlay(QPainter& painter);
    void drawGrips(QPainter& painter);
    void acceptPoint(const SPoint2d& world_point);
    SPoint2d constrainedPoint(const SPoint2d& world_point);
    std::optional<SEntityId> entityAt(const SPoint2d& world_point) const;
    const SEntityRecord* entityById(SEntityId entity_id) const;
    void selectAt(const SPoint2d& world_point);
    void selectInWindow(const SPoint2d& first_corner, const SPoint2d& second_corner,
                        bool is_crossing);
    QRectF entityBounds(const SEntityRecord& entity) const;
    void emitSelectionState();
    void completeMove(const SPoint2d& destination);
    void completeCopy(const SPoint2d& destination);
    void completeRotate(const SPoint2d& destination);
    void completeScale(const SPoint2d& destination);
    void completeMirror(const SPoint2d& axis_end);
    void completeStretch(const SPoint2d& destination);
    void acceptLengthenPoint(const SPoint2d& world_point);
    void acceptFilletPoint(const SPoint2d& world_point);
    void acceptChamferPoint(const SPoint2d& world_point);
    void acceptBlendPoint(const SPoint2d& world_point);
    void acceptAlignPoint(const SPoint2d& world_point);
    void acceptRectangularArrayPoint(const SPoint2d& world_point);
    void acceptPolarArrayPoint(const SPoint2d& world_point);
    void acceptPathArrayPoint(const SPoint2d& world_point);
    void acceptArrayEditPoint(const SPoint2d& world_point);
    bool editAssociativeArray(SAssociativeArrayData parameters);
    void acceptEllipsePoint(const SPoint2d& world_point);
    void acceptSplinePoint(const SPoint2d& world_point);
    void acceptMTextPoint(const SPoint2d& world_point);
    void acceptLeaderPoint(const SPoint2d& world_point);
    void acceptDimensionPoint(const SPoint2d& world_point);
    std::optional<SHatchEntity> hatchFromBoundaryEntity(const SEntityRecord& boundary) const;
    void acceptSplineEditPoint(const SPoint2d& world_point);
    bool handleSplineEditKeyword(const QString& normalized_keyword);
    void showSplineEditPrompt();
    void acceptCircleConstructionPoint(const SPoint2d& world_point);
    void acceptTangentCirclePoint(const SPoint2d& world_point);
    bool tangentCirclePreview(const SPoint2d& candidate_pick, SCircleEntity& result) const;
    void acceptArcConstructionPoint(const SPoint2d& world_point);
    void acceptStandardShapePoint(const SPoint2d& world_point);
    void completeArcConstruction();
    void acceptPolylineEditPoint(const SPoint2d& world_point);
    bool handlePolylineEditKeyword(const QString& normalized_keyword);
    void showPolylineEditPrompt();
    void completePolylineEditJoin();
    void completeOffset(const SPoint2d& through_point);
    void completeJoin();
    void completePolyline(bool is_closed = false);
    bool beginGripDrag(const QPointF& screen_point);
    void updateGripDrag(const SPoint2d& destination);
    void commitGripDrag();
    void cancelGripDrag();
    std::optional<SObjectSnapResult> trackingSnap(const SPoint2d& cursor,
                                                  const std::optional<SPoint2d>& reference_point,
                                                  double tolerance) const;
    double adaptiveGridSpacing() const;
    std::optional<SPoint2d> coordinateReferencePoint() const noexcept;

    QPointer<SCadDocument> m_document;
    SToolMode m_tool_mode = SToolMode::Select;
    SToolMode m_last_tool_mode = SToolMode::Line;
    std::optional<SPoint2d> m_first_point;
    std::optional<SPoint2d> m_command_start_point;
    std::vector<SPoint2d> m_input_points;
    std::vector<double> m_polyline_bulges;
    std::vector<double> m_polyline_start_widths;
    std::vector<double> m_polyline_end_widths;
    std::optional<SPoint2d> m_polyline_arc_point;
    std::optional<std::size_t> m_spline_edit_control_index;
    std::optional<SEntityId> m_selected_entity_id;
    std::vector<SEntityId> m_selected_entity_ids;
    std::vector<SEntityId> m_curve_reference_ids;
    std::optional<SEntityId> m_reference_entity_id;
    std::optional<SObjectSnapResult> m_active_object_snap;
    std::vector<SPoint2d> m_tracking_points;
    std::optional<SGripHandle> m_active_grip;
    std::vector<SEntityRecord> m_grip_sources;
    std::vector<SEntityRecord> m_grip_previews;
    SPoint2d m_grip_reference_point;
    SPoint2d m_cursor_world;
    SPoint2d m_pointer_world;
    std::optional<SPoint2d> m_command_preview_point;
    QPoint m_last_mouse_position;
    QPoint m_selection_start_screen;
    QPoint m_selection_current_screen;
    SPoint2d m_selection_start_world;
    QPointF m_pan_offset;
    double m_zoom = 1.0;
    double m_fillet_radius = 5.0;
    double m_chamfer_first_distance = 5.0;
    double m_chamfer_second_distance = 5.0;
    double m_grid_snap_spacing = 10.0;
    double m_grid_rotation = 0.0;
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
    SGripOperation m_grip_operation = SGripOperation::Stretch;
    SCircleConstruction m_circle_construction = SCircleConstruction::CenterRadius;
    SArcConstruction m_arc_construction = SArcConstruction::ThreePoint;
    SStandardShapeType m_standard_shape_type = SStandardShapeType::FivePointStar;
    std::optional<double> m_arc_construction_parameter;
    bool m_is_additive_selection = false;
    bool m_is_grid_visible = true;
    bool m_is_object_snap_enabled = true;
    SObjectSnapModes m_object_snap_modes = objectSnapModeValue(SObjectSnapMode::Endpoint) |
                                           objectSnapModeValue(SObjectSnapMode::Center);
    bool m_is_ortho_enabled = false;
    bool m_is_grid_snap_enabled = false;
    bool m_is_tracking_enabled = true;
    bool m_is_lineweight_visible = false;
    bool m_is_node_display_visible = false;
    bool m_is_direction_display_visible = false;
    bool m_is_sequence_display_visible = false;
    QColor m_canvas_color{8, 11, 15};
    QColor m_grid_color{31, 40, 51};
    QColor m_major_grid_color{58, 72, 88};
    QColor m_overlay_text_color{183, 192, 202};
    QString m_plot_style_override;
    std::optional<SPlotStyleTable> m_plot_style_table_override;
    QString m_pending_text;
    STextHorizontalAlignment m_pending_text_alignment = STextHorizontalAlignment::Left;
    STextVerticalAlignment m_pending_text_vertical_alignment = STextVerticalAlignment::Baseline;
    SMTextEntity m_pending_mtext;
    QString m_pending_leader_text;
    double m_pending_leader_text_height = 2.5;
    double m_pending_leader_arrow_size = 2.5;
    SHatchEntity m_hatch_settings;
    std::vector<SToolpathMotion> m_simulation_motions;
    std::size_t m_completed_simulation_motion_count = 0;
    bool m_is_simulation_overlay_active = false;
    bool m_is_simulation_trace_visible = true;
};

} // namespace vectorPath
