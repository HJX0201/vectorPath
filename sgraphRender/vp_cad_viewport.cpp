#include "vp_cad_viewport.h"

#include "vp_cad_document.h"
#include "vp_coordinate_input.h"
#include "vp_document_transaction.h"
#include "vp_entity.h"

#include <QKeyEvent>
#include <QKeySequence>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <utility>

namespace Vp
{

VpCadViewport::VpCadViewport(QWidget* parent) : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
    setMinimumSize(480, 320);
}

void VpCadViewport::setDocument(VpCadDocument* document)
{
    if (m_document)
    {
        disconnect(m_document, nullptr, this, nullptr);
    }
    m_document = document;
    m_selected_entity_ids.clear();
    m_selected_entity_id.reset();
    m_drafting_state.clearTrackingPoints();
    emitSelectionState();
    if (m_document)
    {
        connect(m_document, &VpCadDocument::documentChanged, this,
                [this]()
                {
                    const std::size_t previous_size = m_selected_entity_ids.size();
                    m_selected_entity_ids.erase(
                        std::remove_if(m_selected_entity_ids.begin(), m_selected_entity_ids.end(),
                                       [this](VpEntityId entity_id)
                                       {
                                           const VpEntityRecord* entity = entityById(entity_id);
                                           return entity == nullptr ||
                                                  !m_document->isLayerVisible(entity->layer_name) ||
                                                  m_document->isLayerLocked(entity->layer_name);
                                       }),
                        m_selected_entity_ids.end());
                    m_selected_entity_id =
                        m_selected_entity_ids.empty()
                            ? std::optional<VpEntityId>()
                            : std::optional<VpEntityId>(m_selected_entity_ids.front());
                    if (previous_size != m_selected_entity_ids.size())
                    {
                        emitSelectionState();
                    }
                    update();
                });
    }
    update();
}

void VpCadViewport::setToolMode(VpToolMode tool_mode)
{
    if (m_tool_mode == tool_mode && !m_first_point.has_value())
    {
        return;
    }
    m_tool_mode = tool_mode;
    cancelGripDrag();
    m_drafting_state.clearTrackingPoints();
    setFocus(Qt::ShortcutFocusReason);
    if (tool_mode != VpToolMode::Select)
    {
        m_last_tool_mode = tool_mode;
    }
    m_first_point.reset();
    m_command_start_point.reset();
    m_line_segment_count = 0;
    m_input_points.clear();
    m_polyline_bulges.clear();
    m_polyline_start_widths.clear();
    m_polyline_end_widths.clear();
    m_polyline_arc_point.reset();
    m_spline_edit_control_index.reset();
    m_polyline_arc_mode = false;
    m_polyline_start_width = 0.0;
    m_polyline_end_width = 0.0;
    m_reference_entity_id.reset();
    m_curve_reference_ids.clear();
    m_is_additive_selection = false;
    if (tool_mode == VpToolMode::PolylineEdit || tool_mode == VpToolMode::SplineEdit)
    {
        m_selected_entity_ids.erase(
            std::remove_if(m_selected_entity_ids.begin(), m_selected_entity_ids.end(),
                           [this](VpEntityId entity_id)
                           {
                               const VpEntityRecord* entity = entityById(entity_id);
                               return !entity ||
                                      entity->type != (m_tool_mode == VpToolMode::PolylineEdit
                                                           ? VpEntityType::Polyline
                                                           : VpEntityType::Spline);
                           }),
            m_selected_entity_ids.end());
        m_selected_entity_id = m_selected_entity_ids.empty()
                                   ? std::optional<VpEntityId>()
                                   : std::optional<VpEntityId>(m_selected_entity_ids.front());
        emitSelectionState();
    }
    emit toolModeChanged(m_tool_mode);
    switch (m_tool_mode)
    {
    case VpToolMode::Line:
        emit commandMessage(tr("LINE 指定第一点："));
        break;
    case VpToolMode::Circle:
        m_circle_construction = VpCircleConstruction::CenterRadius;
        emit commandMessage(tr("CIRCLE 圆心-半径：指定圆心："));
        break;
    case VpToolMode::Polyline:
        emit commandMessage(tr("PLINE 指定起点："));
        break;
    case VpToolMode::PolylineEdit:
        showPolylineEditPrompt();
        break;
    case VpToolMode::Ellipse:
        emit commandMessage(tr("ELLIPSE 指定第一条轴的第一个端点："));
        break;
    case VpToolMode::Spline:
        emit commandMessage(tr("SPLINE 指定第一个控制点："));
        break;
    case VpToolMode::SplineEdit:
        showSplineEditPrompt();
        break;
    case VpToolMode::Rectangle:
        emit commandMessage(tr("RECTANG 指定第一个角点："));
        break;
    case VpToolMode::StandardShape:
        emit commandMessage(tr("标准图形：指定中心点："));
        break;
    case VpToolMode::Arc:
        m_arc_construction = VpArcConstruction::ThreePoint;
        emit commandMessage(tr("ARC 3P：指定起点："));
        break;
    case VpToolMode::Move:
        emit commandMessage(m_selected_entity_id ? tr("MOVE 指定基点：") : tr("MOVE 选择对象："));
        break;
    case VpToolMode::Copy:
        emit commandMessage(m_selected_entity_id ? tr("COPY 指定基点：") : tr("COPY 选择对象："));
        break;
    case VpToolMode::Rotate:
        emit commandMessage(m_selected_entity_id ? tr("ROTATE 指定基点：")
                                                 : tr("ROTATE 选择对象："));
        break;
    case VpToolMode::Scale:
        emit commandMessage(m_selected_entity_id ? tr("SCALE 指定基点：") : tr("SCALE 选择对象："));
        break;
    case VpToolMode::Mirror:
        emit commandMessage(m_selected_entity_id ? tr("MIRROR 指定镜像线第一点：")
                                                 : tr("MIRROR 选择对象："));
        break;
    case VpToolMode::Erase:
        emit commandMessage(tr("ERASE 选择要删除的对象："));
        break;
    case VpToolMode::Trim:
        emit commandMessage(tr("TRIM 选择直线剪切边："));
        break;
    case VpToolMode::Extend:
        emit commandMessage(tr("EXTEND 选择直线边界："));
        break;
    case VpToolMode::Break:
        emit commandMessage(tr("BREAK 选择直线、圆或圆弧和第一个打断点："));
        break;
    case VpToolMode::Join:
        emit commandMessage(m_selected_entity_ids.size() >= 2
                                ? tr("JOIN 按 Enter 合并当前选择集，或继续选择同类实体：")
                                : tr("JOIN 依次选择相连直线、开放多段线或同圆圆弧，Enter 完成："));
        break;
    case VpToolMode::Explode:
        emit commandMessage(tr("EXPLODE 选择多段线或填充边界："));
        break;
    case VpToolMode::Stretch:
        emit commandMessage(tr("STRETCH 指定交叉窗口第一个角点："));
        break;
    case VpToolMode::Lengthen:
        emit commandMessage(tr("LENGTHEN 选择直线、圆弧或开放多段线的端部："));
        break;
    case VpToolMode::Fillet:
        emit commandMessage(
            tr("FILLET 当前半径 %1。选择直线、圆、圆弧或多段线，或输入 FILLET R 数值：")
                .arg(m_fillet_radius, 0, 'f', 2));
        break;
    case VpToolMode::Chamfer:
        emit commandMessage(
            tr("CHAMFER 当前距离 %1, %2。选择直线、圆弧或多段线，或输入 CHAMFER D 数值 数值：")
                .arg(m_chamfer_first_distance, 0, 'f', 2)
                .arg(m_chamfer_second_distance, 0, 'f', 2));
        break;
    case VpToolMode::Blend:
        emit commandMessage(tr("BLEND 在第一条直线、圆弧或样条曲线的端部附近拾取："));
        break;
    case VpToolMode::Align:
        emit commandMessage(m_selected_entity_ids.empty() ? tr("ALIGN 选择对象：")
                                                          : tr("ALIGN 指定第一个源点："));
        break;
    case VpToolMode::ArrayRect:
        emit commandMessage(m_selected_entity_ids.empty()
                                ? tr("ARRAYRECT 当前 %1 列 × %2 行。选择对象：")
                                      .arg(m_array_column_count)
                                      .arg(m_array_row_count)
                                : tr("ARRAYRECT 当前 %1 列 × %2 行。指定基点：")
                                      .arg(m_array_column_count)
                                      .arg(m_array_row_count));
        break;
    case VpToolMode::ArrayPolar:
        emit commandMessage(m_selected_entity_ids.empty()
                                ? tr("ARRAYPOLAR 当前 %1 项，填充角 %2°。选择对象：")
                                      .arg(m_array_polar_item_count)
                                      .arg(m_array_polar_fill_angle, 0, 'f', 1)
                                : tr("ARRAYPOLAR 当前 %1 项，填充角 %2°。指定中心点：")
                                      .arg(m_array_polar_item_count)
                                      .arg(m_array_polar_fill_angle, 0, 'f', 1));
        break;
    case VpToolMode::ArrayPath:
        emit commandMessage(m_selected_entity_ids.empty()
                                ? tr("ARRAYPATH 当前 %1 项，%2。选择源对象：")
                                      .arg(m_array_path_item_count)
                                      .arg(m_array_path_align ? tr("沿路径对齐") : tr("保持方向"))
                                : tr("ARRAYPATH 当前 %1 项，%2。指定源基点：")
                                      .arg(m_array_path_item_count)
                                      .arg(m_array_path_align ? tr("沿路径对齐") : tr("保持方向")));
        break;
    case VpToolMode::ArrayEdit:
        emit commandMessage(m_selected_entity_ids.empty()
                                ? tr("ARRAYEDIT 请选择关联阵列中的任意实体：")
                                : tr("ARRAYEDIT 输入 RECT 列 行 列距 行距、POLAR 项目数 填充角，"
                                     "或 PATH 项目数 ALIGN|NOALIGN。"));
        break;
    case VpToolMode::Offset:
        emit commandMessage(m_selected_entity_id ? tr("OFFSET 指定通过点：")
                                                 : tr("OFFSET 选择直线、圆、圆弧、多段线或样条："));
        break;
    case VpToolMode::Text:
        emit commandMessage(tr("TEXT 指定插入点："));
        break;
    case VpToolMode::MText:
        emit commandMessage(tr("MTEXT 指定插入点："));
        break;
    case VpToolMode::Leader:
        emit commandMessage(tr("MLEADER 指定箭头位置："));
        break;
    case VpToolMode::LinearDimension:
        emit commandMessage(tr("DIMLINEAR 指定第一条尺寸界线原点："));
        break;
    case VpToolMode::AlignedDimension:
        emit commandMessage(tr("DIMALIGNED 指定第一条尺寸界线原点："));
        break;
    case VpToolMode::AngularDimension:
        emit commandMessage(tr("DIMANGULAR 指定角点："));
        break;
    case VpToolMode::RadiusDimension:
        emit commandMessage(tr("DIMRADIUS 指定圆心："));
        break;
    case VpToolMode::DiameterDimension:
        emit commandMessage(tr("DIMDIAMETER 指定圆心："));
        break;
    case VpToolMode::ArcLengthDimension:
        emit commandMessage(tr("DIMARC 指定圆心："));
        break;
    case VpToolMode::OrdinateDimension:
        emit commandMessage(tr("DIMORDINATE 指定原点："));
        break;
    case VpToolMode::Hatch:
        emit commandMessage(tr("HATCH 选择闭合多段线边界："));
        break;
    case VpToolMode::Select:
        emit commandMessage(tr("就绪"));
        break;
    }
    update();
}

VpToolMode VpCadViewport::toolMode() const noexcept
{
    return m_tool_mode;
}

void VpCadViewport::cancelCommand()
{
    cancelGripDrag();
    m_drafting_state.clearTrackingPoints();
    m_first_point.reset();
    m_command_start_point.reset();
    m_line_segment_count = 0;
    m_input_points.clear();
    m_polyline_bulges.clear();
    m_polyline_start_widths.clear();
    m_polyline_end_widths.clear();
    m_polyline_arc_point.reset();
    m_spline_edit_control_index.reset();
    m_polyline_arc_mode = false;
    m_curve_reference_ids.clear();
    m_tool_mode = VpToolMode::Select;
    emit toolModeChanged(m_tool_mode);
    emit commandMessage(tr("*取消*"));
    update();
}

void VpCadViewport::setGridVisible(bool is_visible)
{
    if (m_is_grid_visible == is_visible)
    {
        return;
    }
    m_is_grid_visible = is_visible;
    emit gridVisibilityChanged(m_is_grid_visible);
    update();
}

bool VpCadViewport::isGridVisible() const noexcept
{
    return m_is_grid_visible;
}

void VpCadViewport::setObjectSnapEnabled(bool is_enabled)
{
    const VpObjectSnapModes target_modes = is_enabled
                                               ? objectSnapModeValue(VpObjectSnapMode::Endpoint) |
                                                     objectSnapModeValue(VpObjectSnapMode::Center)
                                               : 0;
    if (m_object_snap_modes == target_modes)
    {
        return;
    }
    m_object_snap_modes = target_modes;
    m_is_object_snap_enabled = is_enabled;
    if (!m_is_object_snap_enabled)
    {
        m_active_object_snap.reset();
    }
    emit objectSnapChanged(m_is_object_snap_enabled);
    emit objectSnapModesChanged(isEndpointSnapEnabled(), isCenterSnapEnabled());
    update();
}

bool VpCadViewport::isObjectSnapEnabled() const noexcept
{
    return m_is_object_snap_enabled;
}

void VpCadViewport::setEndpointSnapEnabled(bool is_enabled)
{
    const VpObjectSnapModes endpoint = objectSnapModeValue(VpObjectSnapMode::Endpoint);
    const VpObjectSnapModes modes =
        is_enabled ? static_cast<VpObjectSnapModes>(m_object_snap_modes | endpoint)
                   : static_cast<VpObjectSnapModes>(m_object_snap_modes & ~endpoint);
    if (modes == m_object_snap_modes)
    {
        return;
    }
    m_object_snap_modes = modes;
    m_is_object_snap_enabled = m_object_snap_modes != 0;
    emit objectSnapChanged(m_is_object_snap_enabled);
    emit objectSnapModesChanged(isEndpointSnapEnabled(), isCenterSnapEnabled());
    update();
}

bool VpCadViewport::isEndpointSnapEnabled() const noexcept
{
    return objectSnapModeEnabled(m_object_snap_modes, VpObjectSnapMode::Endpoint);
}

void VpCadViewport::setCenterSnapEnabled(bool is_enabled)
{
    const VpObjectSnapModes center = objectSnapModeValue(VpObjectSnapMode::Center);
    const VpObjectSnapModes modes =
        is_enabled ? static_cast<VpObjectSnapModes>(m_object_snap_modes | center)
                   : static_cast<VpObjectSnapModes>(m_object_snap_modes & ~center);
    if (modes == m_object_snap_modes)
    {
        return;
    }
    m_object_snap_modes = modes;
    m_is_object_snap_enabled = m_object_snap_modes != 0;
    emit objectSnapChanged(m_is_object_snap_enabled);
    emit objectSnapModesChanged(isEndpointSnapEnabled(), isCenterSnapEnabled());
    update();
}

bool VpCadViewport::isCenterSnapEnabled() const noexcept
{
    return objectSnapModeEnabled(m_object_snap_modes, VpObjectSnapMode::Center);
}

void VpCadViewport::setOrthoEnabled(bool is_enabled)
{
    if (!m_drafting_state.setOrthoEnabled(is_enabled))
    {
        return;
    }
    emit orthoChanged(is_enabled);
    update();
}

bool VpCadViewport::isOrthoEnabled() const noexcept
{
    return m_drafting_state.isOrthoEnabled();
}

void VpCadViewport::setLineweightVisible(bool is_visible)
{
    m_is_lineweight_visible = is_visible;
    update();
}

bool VpCadViewport::isLineweightVisible() const noexcept
{
    return m_is_lineweight_visible;
}

void VpCadViewport::setNodeDisplayVisible(bool is_visible)
{
    if (m_is_node_display_visible == is_visible)
    {
        return;
    }
    m_is_node_display_visible = is_visible;
    emit entityDisplayOptionsChanged(m_is_node_display_visible, m_is_direction_display_visible,
                                     m_is_sequence_display_visible);
    update();
}

bool VpCadViewport::isNodeDisplayVisible() const noexcept
{
    return m_is_node_display_visible;
}

void VpCadViewport::setDirectionDisplayVisible(bool is_visible)
{
    if (m_is_direction_display_visible == is_visible)
    {
        return;
    }
    m_is_direction_display_visible = is_visible;
    emit entityDisplayOptionsChanged(m_is_node_display_visible, m_is_direction_display_visible,
                                     m_is_sequence_display_visible);
    update();
}

bool VpCadViewport::isDirectionDisplayVisible() const noexcept
{
    return m_is_direction_display_visible;
}

void VpCadViewport::setSequenceDisplayVisible(bool is_visible)
{
    if (m_is_sequence_display_visible == is_visible)
    {
        return;
    }
    m_is_sequence_display_visible = is_visible;
    emit entityDisplayOptionsChanged(m_is_node_display_visible, m_is_direction_display_visible,
                                     m_is_sequence_display_visible);
    update();
}

bool VpCadViewport::isSequenceDisplayVisible() const noexcept
{
    return m_is_sequence_display_visible;
}

std::optional<VpEntityId> VpCadViewport::selectedEntityId() const noexcept
{
    return m_selected_entity_id;
}

QVector<quint64> VpCadViewport::selectedEntityIds() const
{
    QVector<quint64> result;
    result.reserve(static_cast<int>(m_selected_entity_ids.size()));
    for (VpEntityId entity_id : m_selected_entity_ids)
    {
        result.append(static_cast<quint64>(entity_id));
    }
    return result;
}

void VpCadViewport::selectAll()
{
    cancelGripDrag();
    m_selected_entity_ids.clear();
    if (m_document)
    {
        m_selected_entity_ids.reserve(m_document->entities().size());
        for (const VpEntityRecord& entity : m_document->entities())
        {
            if (m_document->isLayerVisible(entity.layer_name) &&
                !m_document->isLayerLocked(entity.layer_name))
            {
                m_selected_entity_ids.push_back(entity.id);
            }
        }
    }
    m_selected_entity_id = m_selected_entity_ids.empty()
                               ? std::optional<VpEntityId>()
                               : std::optional<VpEntityId>(m_selected_entity_ids.front());
    emitSelectionState();
    emit commandMessage(tr("已选择 %1 个实体。").arg(m_selected_entity_ids.size()));
    update();
}

void VpCadViewport::clearSelection()
{
    cancelGripDrag();
    m_selected_entity_ids.clear();
    m_selected_entity_id.reset();
    emitSelectionState();
    update();
}

void VpCadViewport::deleteSelected()
{
    if (!m_document || m_selected_entity_ids.empty())
    {
        return;
    }
    auto transaction = m_document->beginTransaction(tr("删除选择集"));
    for (VpEntityId entity_id : m_selected_entity_ids)
    {
        transaction->removeEntity(entity_id);
    }
    transaction->commit();
    const std::size_t removed_count = m_selected_entity_ids.size();
    clearSelection();
    emit commandMessage(tr("已删除 %1 个实体。").arg(removed_count));
}

void VpCadViewport::beginGripEdit(VpGripOperation operation)
{
    setGripOperation(operation);
    setToolMode(VpToolMode::Select);
    emit commandMessage(m_selected_entity_ids.empty()
                            ? tr("夹点编辑：选择对象，然后拖动蓝色夹点；Space 循环模式：")
                            : tr("夹点编辑：拖动蓝色夹点；Space 循环模式，Esc 取消："));
}

void VpCadViewport::setGripOperation(VpGripOperation operation)
{
    if (m_grip_operation == operation)
    {
        return;
    }
    m_grip_operation = operation;
    emit gripOperationChanged(m_grip_operation);
    if (m_is_grip_dragging)
    {
        updateGripDrag(m_cursor_world);
    }
}

VpGripOperation VpCadViewport::gripOperation() const noexcept
{
    return m_grip_operation;
}

void VpCadViewport::cycleGripOperation()
{
    switch (m_grip_operation)
    {
    case VpGripOperation::Stretch:
        setGripOperation(VpGripOperation::Move);
        break;
    case VpGripOperation::Move:
        setGripOperation(VpGripOperation::Rotate);
        break;
    case VpGripOperation::Rotate:
        setGripOperation(VpGripOperation::Scale);
        break;
    case VpGripOperation::Scale:
        setGripOperation(VpGripOperation::Mirror);
        break;
    case VpGripOperation::Mirror:
        setGripOperation(VpGripOperation::Stretch);
        break;
    }
    QString operation_name;
    switch (m_grip_operation)
    {
    case VpGripOperation::Stretch:
        operation_name = tr("拉伸");
        break;
    case VpGripOperation::Move:
        operation_name = tr("移动");
        break;
    case VpGripOperation::Rotate:
        operation_name = tr("旋转");
        break;
    case VpGripOperation::Scale:
        operation_name = tr("缩放");
        break;
    case VpGripOperation::Mirror:
        operation_name = tr("镜像");
        break;
    }
    emit commandMessage(tr("夹点模式：%1。指定目标点或按 Space 继续循环。").arg(operation_name));
}

void VpCadViewport::selectEntitiesInWindow(const VpPoint2d& first_corner,
                                           const VpPoint2d& second_corner, bool is_crossing,
                                           bool is_additive)
{
    m_is_additive_selection = is_additive;
    selectInWindow(first_corner, second_corner, is_crossing);
}

void VpCadViewport::setPendingText(QString text)
{
    m_pending_text = std::move(text);
}

void VpCadViewport::setPendingTextAlignment(VpTextHorizontalAlignment alignment,
                                            VpTextVerticalAlignment vertical_alignment)
{
    m_pending_text_alignment = alignment;
    m_pending_text_vertical_alignment = vertical_alignment;
}

void VpCadViewport::submitWorldPoint(const VpPoint2d& world_point)
{
    clearCoordinateInputPreview();
    acceptPoint(world_point);
}

bool VpCadViewport::submitCoordinateInput(const VpCoordinateInput& input)
{
    const std::optional<VpPoint2d> world_point =
        resolveCoordinateInput(input, coordinateReferencePoint());
    if (!world_point)
    {
        clearCoordinateInputPreview();
        return false;
    }
    submitWorldPoint(*world_point);
    return true;
}

bool VpCadViewport::previewCoordinateInput(const VpCoordinateInput& input)
{
    const std::optional<VpPoint2d> world_point =
        resolveCoordinateInput(input, coordinateReferencePoint());
    if (!world_point)
    {
        clearCoordinateInputPreview();
        return false;
    }
    m_command_preview_point = world_point;
    m_cursor_world = *world_point;
    emit cursorWorldPositionChanged(m_cursor_world);
    update();
    return true;
}

void VpCadViewport::clearCoordinateInputPreview()
{
    if (!m_command_preview_point)
    {
        return;
    }
    m_command_preview_point.reset();
    m_cursor_world = m_pointer_world;
    emit cursorWorldPositionChanged(m_cursor_world);
    update();
}

std::optional<VpPoint2d> VpCadViewport::commandPreviewPoint() const noexcept
{
    return m_command_preview_point;
}

std::optional<VpPoint2d> VpCadViewport::coordinateReferencePoint() const noexcept
{
    if (!m_input_points.empty())
    {
        return m_input_points.back();
    }
    return m_first_point;
}

} // namespace Vp
