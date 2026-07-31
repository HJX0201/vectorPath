#include "s_cad_viewport.h"

#include "s_cad_document.h"
#include "s_coordinate_input.h"
#include "s_document_transaction.h"
#include "s_entity.h"

#include <QKeyEvent>
#include <QKeySequence>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <utility>

namespace vectorPath
{

SCadViewport::SCadViewport(QWidget* parent) : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
    setMinimumSize(480, 320);
}

void SCadViewport::setDocument(SCadDocument* document)
{
    if (m_document)
    {
        disconnect(m_document, nullptr, this, nullptr);
    }
    m_document = document;
    m_selected_entity_ids.clear();
    m_selected_entity_id.reset();
    m_tracking_points.clear();
    emitSelectionState();
    if (m_document)
    {
        connect(m_document, &SCadDocument::documentChanged, this,
                [this]()
                {
                    const std::size_t previous_size = m_selected_entity_ids.size();
                    m_selected_entity_ids.erase(
                        std::remove_if(m_selected_entity_ids.begin(), m_selected_entity_ids.end(),
                                       [this](SEntityId entity_id)
                                       {
                                           const SEntityRecord* entity = entityById(entity_id);
                                           return entity == nullptr ||
                                                  !m_document->isLayerVisible(entity->layer_name) ||
                                                  m_document->isLayerLocked(entity->layer_name);
                                       }),
                        m_selected_entity_ids.end());
                    m_selected_entity_id =
                        m_selected_entity_ids.empty()
                            ? std::optional<SEntityId>()
                            : std::optional<SEntityId>(m_selected_entity_ids.front());
                    if (previous_size != m_selected_entity_ids.size())
                    {
                        emitSelectionState();
                    }
                    update();
                });
    }
    update();
}

void SCadViewport::setToolMode(SToolMode tool_mode)
{
    if (m_tool_mode == tool_mode && !m_first_point.has_value())
    {
        return;
    }
    m_tool_mode = tool_mode;
    cancelGripDrag();
    m_tracking_points.clear();
    setFocus(Qt::ShortcutFocusReason);
    if (tool_mode != SToolMode::Select)
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
    if (tool_mode == SToolMode::PolylineEdit || tool_mode == SToolMode::SplineEdit)
    {
        m_selected_entity_ids.erase(
            std::remove_if(m_selected_entity_ids.begin(), m_selected_entity_ids.end(),
                           [this](SEntityId entity_id)
                           {
                               const SEntityRecord* entity = entityById(entity_id);
                               return !entity ||
                                      entity->type != (m_tool_mode == SToolMode::PolylineEdit
                                                           ? SEntityType::Polyline
                                                           : SEntityType::Spline);
                           }),
            m_selected_entity_ids.end());
        m_selected_entity_id = m_selected_entity_ids.empty()
                                   ? std::optional<SEntityId>()
                                   : std::optional<SEntityId>(m_selected_entity_ids.front());
        emitSelectionState();
    }
    emit toolModeChanged(m_tool_mode);
    switch (m_tool_mode)
    {
    case SToolMode::Line:
        emit commandMessage(tr("LINE 指定第一点："));
        break;
    case SToolMode::Circle:
        m_circle_construction = SCircleConstruction::CenterRadius;
        emit commandMessage(tr("CIRCLE 圆心-半径：指定圆心："));
        break;
    case SToolMode::Polyline:
        emit commandMessage(tr("PLINE 指定起点："));
        break;
    case SToolMode::PolylineEdit:
        showPolylineEditPrompt();
        break;
    case SToolMode::Ellipse:
        emit commandMessage(tr("ELLIPSE 指定第一条轴的第一个端点："));
        break;
    case SToolMode::Spline:
        emit commandMessage(tr("SPLINE 指定第一个控制点："));
        break;
    case SToolMode::SplineEdit:
        showSplineEditPrompt();
        break;
    case SToolMode::Rectangle:
        emit commandMessage(tr("RECTANG 指定第一个角点："));
        break;
    case SToolMode::StandardShape:
        emit commandMessage(tr("标准图形：指定中心点："));
        break;
    case SToolMode::Arc:
        m_arc_construction = SArcConstruction::ThreePoint;
        emit commandMessage(tr("ARC 3P：指定起点："));
        break;
    case SToolMode::Move:
        emit commandMessage(m_selected_entity_id ? tr("MOVE 指定基点：") : tr("MOVE 选择对象："));
        break;
    case SToolMode::Copy:
        emit commandMessage(m_selected_entity_id ? tr("COPY 指定基点：") : tr("COPY 选择对象："));
        break;
    case SToolMode::Rotate:
        emit commandMessage(m_selected_entity_id ? tr("ROTATE 指定基点：")
                                                 : tr("ROTATE 选择对象："));
        break;
    case SToolMode::Scale:
        emit commandMessage(m_selected_entity_id ? tr("SCALE 指定基点：") : tr("SCALE 选择对象："));
        break;
    case SToolMode::Mirror:
        emit commandMessage(m_selected_entity_id ? tr("MIRROR 指定镜像线第一点：")
                                                 : tr("MIRROR 选择对象："));
        break;
    case SToolMode::Erase:
        emit commandMessage(tr("ERASE 选择要删除的对象："));
        break;
    case SToolMode::Trim:
        emit commandMessage(tr("TRIM 选择直线剪切边："));
        break;
    case SToolMode::Extend:
        emit commandMessage(tr("EXTEND 选择直线边界："));
        break;
    case SToolMode::Break:
        emit commandMessage(tr("BREAK 选择直线、圆或圆弧和第一个打断点："));
        break;
    case SToolMode::Join:
        emit commandMessage(m_selected_entity_ids.size() >= 2
                                ? tr("JOIN 按 Enter 合并当前选择集，或继续选择同类实体：")
                                : tr("JOIN 依次选择相连直线、开放多段线或同圆圆弧，Enter 完成："));
        break;
    case SToolMode::Explode:
        emit commandMessage(tr("EXPLODE 选择多段线或填充边界："));
        break;
    case SToolMode::Stretch:
        emit commandMessage(tr("STRETCH 指定交叉窗口第一个角点："));
        break;
    case SToolMode::Lengthen:
        emit commandMessage(tr("LENGTHEN 选择直线、圆弧或开放多段线的端部："));
        break;
    case SToolMode::Fillet:
        emit commandMessage(
            tr("FILLET 当前半径 %1。选择直线、圆、圆弧或多段线，或输入 FILLET R 数值：")
                .arg(m_fillet_radius, 0, 'f', 2));
        break;
    case SToolMode::Chamfer:
        emit commandMessage(
            tr("CHAMFER 当前距离 %1, %2。选择直线、圆弧或多段线，或输入 CHAMFER D 数值 数值：")
                .arg(m_chamfer_first_distance, 0, 'f', 2)
                .arg(m_chamfer_second_distance, 0, 'f', 2));
        break;
    case SToolMode::Blend:
        emit commandMessage(tr("BLEND 在第一条直线、圆弧或样条曲线的端部附近拾取："));
        break;
    case SToolMode::Align:
        emit commandMessage(m_selected_entity_ids.empty() ? tr("ALIGN 选择对象：")
                                                          : tr("ALIGN 指定第一个源点："));
        break;
    case SToolMode::ArrayRect:
        emit commandMessage(m_selected_entity_ids.empty()
                                ? tr("ARRAYRECT 当前 %1 列 × %2 行。选择对象：")
                                      .arg(m_array_column_count)
                                      .arg(m_array_row_count)
                                : tr("ARRAYRECT 当前 %1 列 × %2 行。指定基点：")
                                      .arg(m_array_column_count)
                                      .arg(m_array_row_count));
        break;
    case SToolMode::ArrayPolar:
        emit commandMessage(m_selected_entity_ids.empty()
                                ? tr("ARRAYPOLAR 当前 %1 项，填充角 %2°。选择对象：")
                                      .arg(m_array_polar_item_count)
                                      .arg(m_array_polar_fill_angle, 0, 'f', 1)
                                : tr("ARRAYPOLAR 当前 %1 项，填充角 %2°。指定中心点：")
                                      .arg(m_array_polar_item_count)
                                      .arg(m_array_polar_fill_angle, 0, 'f', 1));
        break;
    case SToolMode::ArrayPath:
        emit commandMessage(m_selected_entity_ids.empty()
                                ? tr("ARRAYPATH 当前 %1 项，%2。选择源对象：")
                                      .arg(m_array_path_item_count)
                                      .arg(m_array_path_align ? tr("沿路径对齐") : tr("保持方向"))
                                : tr("ARRAYPATH 当前 %1 项，%2。指定源基点：")
                                      .arg(m_array_path_item_count)
                                      .arg(m_array_path_align ? tr("沿路径对齐") : tr("保持方向")));
        break;
    case SToolMode::ArrayEdit:
        emit commandMessage(m_selected_entity_ids.empty()
                                ? tr("ARRAYEDIT 请选择关联阵列中的任意实体：")
                                : tr("ARRAYEDIT 输入 RECT 列 行 列距 行距、POLAR 项目数 填充角，"
                                     "或 PATH 项目数 ALIGN|NOALIGN。"));
        break;
    case SToolMode::Offset:
        emit commandMessage(m_selected_entity_id ? tr("OFFSET 指定通过点：")
                                                 : tr("OFFSET 选择直线、圆、圆弧、多段线或样条："));
        break;
    case SToolMode::Text:
        emit commandMessage(tr("TEXT 指定插入点："));
        break;
    case SToolMode::MText:
        emit commandMessage(tr("MTEXT 指定插入点："));
        break;
    case SToolMode::Leader:
        emit commandMessage(tr("MLEADER 指定箭头位置："));
        break;
    case SToolMode::LinearDimension:
        emit commandMessage(tr("DIMLINEAR 指定第一条尺寸界线原点："));
        break;
    case SToolMode::AlignedDimension:
        emit commandMessage(tr("DIMALIGNED 指定第一条尺寸界线原点："));
        break;
    case SToolMode::AngularDimension:
        emit commandMessage(tr("DIMANGULAR 指定角点："));
        break;
    case SToolMode::RadiusDimension:
        emit commandMessage(tr("DIMRADIUS 指定圆心："));
        break;
    case SToolMode::DiameterDimension:
        emit commandMessage(tr("DIMDIAMETER 指定圆心："));
        break;
    case SToolMode::ArcLengthDimension:
        emit commandMessage(tr("DIMARC 指定圆心："));
        break;
    case SToolMode::OrdinateDimension:
        emit commandMessage(tr("DIMORDINATE 指定原点："));
        break;
    case SToolMode::Hatch:
        emit commandMessage(tr("HATCH 选择闭合多段线边界："));
        break;
    case SToolMode::Select:
        emit commandMessage(tr("就绪"));
        break;
    }
    update();
}

SToolMode SCadViewport::toolMode() const noexcept
{
    return m_tool_mode;
}

void SCadViewport::cancelCommand()
{
    cancelGripDrag();
    m_tracking_points.clear();
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
    m_tool_mode = SToolMode::Select;
    emit toolModeChanged(m_tool_mode);
    emit commandMessage(tr("*取消*"));
    update();
}

void SCadViewport::setGridVisible(bool is_visible)
{
    if (m_is_grid_visible == is_visible)
    {
        return;
    }
    m_is_grid_visible = is_visible;
    emit gridVisibilityChanged(m_is_grid_visible);
    update();
}

bool SCadViewport::isGridVisible() const noexcept
{
    return m_is_grid_visible;
}

void SCadViewport::setObjectSnapEnabled(bool is_enabled)
{
    const SObjectSnapModes target_modes =
        is_enabled ? objectSnapModeValue(SObjectSnapMode::Endpoint) |
                         objectSnapModeValue(SObjectSnapMode::Center)
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

bool SCadViewport::isObjectSnapEnabled() const noexcept
{
    return m_is_object_snap_enabled;
}

void SCadViewport::setEndpointSnapEnabled(bool is_enabled)
{
    const SObjectSnapModes endpoint = objectSnapModeValue(SObjectSnapMode::Endpoint);
    const SObjectSnapModes modes =
        is_enabled ? static_cast<SObjectSnapModes>(m_object_snap_modes | endpoint)
                   : static_cast<SObjectSnapModes>(m_object_snap_modes & ~endpoint);
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

bool SCadViewport::isEndpointSnapEnabled() const noexcept
{
    return objectSnapModeEnabled(m_object_snap_modes, SObjectSnapMode::Endpoint);
}

void SCadViewport::setCenterSnapEnabled(bool is_enabled)
{
    const SObjectSnapModes center = objectSnapModeValue(SObjectSnapMode::Center);
    const SObjectSnapModes modes =
        is_enabled ? static_cast<SObjectSnapModes>(m_object_snap_modes | center)
                   : static_cast<SObjectSnapModes>(m_object_snap_modes & ~center);
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

bool SCadViewport::isCenterSnapEnabled() const noexcept
{
    return objectSnapModeEnabled(m_object_snap_modes, SObjectSnapMode::Center);
}

void SCadViewport::setOrthoEnabled(bool is_enabled)
{
    if (m_is_ortho_enabled == is_enabled)
    {
        return;
    }
    m_is_ortho_enabled = is_enabled;
    emit orthoChanged(m_is_ortho_enabled);
    update();
}

bool SCadViewport::isOrthoEnabled() const noexcept
{
    return m_is_ortho_enabled;
}

void SCadViewport::setLineweightVisible(bool is_visible)
{
    m_is_lineweight_visible = is_visible;
    update();
}

bool SCadViewport::isLineweightVisible() const noexcept
{
    return m_is_lineweight_visible;
}

void SCadViewport::setNodeDisplayVisible(bool is_visible)
{
    if (m_is_node_display_visible == is_visible)
    {
        return;
    }
    m_is_node_display_visible = is_visible;
    emit entityDisplayOptionsChanged(m_is_node_display_visible,
                                     m_is_direction_display_visible,
                                     m_is_sequence_display_visible);
    update();
}

bool SCadViewport::isNodeDisplayVisible() const noexcept
{
    return m_is_node_display_visible;
}

void SCadViewport::setDirectionDisplayVisible(bool is_visible)
{
    if (m_is_direction_display_visible == is_visible)
    {
        return;
    }
    m_is_direction_display_visible = is_visible;
    emit entityDisplayOptionsChanged(m_is_node_display_visible,
                                     m_is_direction_display_visible,
                                     m_is_sequence_display_visible);
    update();
}

bool SCadViewport::isDirectionDisplayVisible() const noexcept
{
    return m_is_direction_display_visible;
}

void SCadViewport::setSequenceDisplayVisible(bool is_visible)
{
    if (m_is_sequence_display_visible == is_visible)
    {
        return;
    }
    m_is_sequence_display_visible = is_visible;
    emit entityDisplayOptionsChanged(m_is_node_display_visible,
                                     m_is_direction_display_visible,
                                     m_is_sequence_display_visible);
    update();
}

bool SCadViewport::isSequenceDisplayVisible() const noexcept
{
    return m_is_sequence_display_visible;
}

std::optional<SEntityId> SCadViewport::selectedEntityId() const noexcept
{
    return m_selected_entity_id;
}

QVector<quint64> SCadViewport::selectedEntityIds() const
{
    QVector<quint64> result;
    result.reserve(static_cast<int>(m_selected_entity_ids.size()));
    for (SEntityId entity_id : m_selected_entity_ids)
    {
        result.append(static_cast<quint64>(entity_id));
    }
    return result;
}

void SCadViewport::selectAll()
{
    cancelGripDrag();
    m_selected_entity_ids.clear();
    if (m_document)
    {
        m_selected_entity_ids.reserve(m_document->entities().size());
        for (const SEntityRecord& entity : m_document->entities())
        {
            if (m_document->isLayerVisible(entity.layer_name) &&
                !m_document->isLayerLocked(entity.layer_name))
            {
                m_selected_entity_ids.push_back(entity.id);
            }
        }
    }
    m_selected_entity_id = m_selected_entity_ids.empty()
                               ? std::optional<SEntityId>()
                               : std::optional<SEntityId>(m_selected_entity_ids.front());
    emitSelectionState();
    emit commandMessage(tr("已选择 %1 个实体。").arg(m_selected_entity_ids.size()));
    update();
}

void SCadViewport::clearSelection()
{
    cancelGripDrag();
    m_selected_entity_ids.clear();
    m_selected_entity_id.reset();
    emitSelectionState();
    update();
}

void SCadViewport::deleteSelected()
{
    if (!m_document || m_selected_entity_ids.empty())
    {
        return;
    }
    auto transaction = m_document->beginTransaction(tr("删除选择集"));
    for (SEntityId entity_id : m_selected_entity_ids)
    {
        transaction->removeEntity(entity_id);
    }
    transaction->commit();
    const std::size_t removed_count = m_selected_entity_ids.size();
    clearSelection();
    emit commandMessage(tr("已删除 %1 个实体。").arg(removed_count));
}

void SCadViewport::beginGripEdit(SGripOperation operation)
{
    setGripOperation(operation);
    setToolMode(SToolMode::Select);
    emit commandMessage(m_selected_entity_ids.empty()
                            ? tr("夹点编辑：选择对象，然后拖动蓝色夹点；Space 循环模式：")
                            : tr("夹点编辑：拖动蓝色夹点；Space 循环模式，Esc 取消："));
}

void SCadViewport::setGripOperation(SGripOperation operation)
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

SGripOperation SCadViewport::gripOperation() const noexcept
{
    return m_grip_operation;
}

void SCadViewport::cycleGripOperation()
{
    switch (m_grip_operation)
    {
    case SGripOperation::Stretch:
        setGripOperation(SGripOperation::Move);
        break;
    case SGripOperation::Move:
        setGripOperation(SGripOperation::Rotate);
        break;
    case SGripOperation::Rotate:
        setGripOperation(SGripOperation::Scale);
        break;
    case SGripOperation::Scale:
        setGripOperation(SGripOperation::Mirror);
        break;
    case SGripOperation::Mirror:
        setGripOperation(SGripOperation::Stretch);
        break;
    }
    QString operation_name;
    switch (m_grip_operation)
    {
    case SGripOperation::Stretch:
        operation_name = tr("拉伸");
        break;
    case SGripOperation::Move:
        operation_name = tr("移动");
        break;
    case SGripOperation::Rotate:
        operation_name = tr("旋转");
        break;
    case SGripOperation::Scale:
        operation_name = tr("缩放");
        break;
    case SGripOperation::Mirror:
        operation_name = tr("镜像");
        break;
    }
    emit commandMessage(tr("夹点模式：%1。指定目标点或按 Space 继续循环。").arg(operation_name));
}

void SCadViewport::selectEntitiesInWindow(const SPoint2d& first_corner,
                                          const SPoint2d& second_corner, bool is_crossing,
                                          bool is_additive)
{
    m_is_additive_selection = is_additive;
    selectInWindow(first_corner, second_corner, is_crossing);
}

void SCadViewport::setPendingText(QString text)
{
    m_pending_text = std::move(text);
}

void SCadViewport::setPendingTextAlignment(STextHorizontalAlignment alignment,
                                           STextVerticalAlignment vertical_alignment)
{
    m_pending_text_alignment = alignment;
    m_pending_text_vertical_alignment = vertical_alignment;
}

void SCadViewport::submitWorldPoint(const SPoint2d& world_point)
{
    clearCoordinateInputPreview();
    acceptPoint(world_point);
}

bool SCadViewport::submitCoordinateInput(const SCoordinateInput& input)
{
    const std::optional<SPoint2d> world_point =
        resolveCoordinateInput(input, coordinateReferencePoint());
    if (!world_point)
    {
        clearCoordinateInputPreview();
        return false;
    }
    submitWorldPoint(*world_point);
    return true;
}

bool SCadViewport::previewCoordinateInput(const SCoordinateInput& input)
{
    const std::optional<SPoint2d> world_point =
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

void SCadViewport::clearCoordinateInputPreview()
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

std::optional<SPoint2d> SCadViewport::commandPreviewPoint() const noexcept
{
    return m_command_preview_point;
}

std::optional<SPoint2d> SCadViewport::coordinateReferencePoint() const noexcept
{
    if (!m_input_points.empty())
    {
        return m_input_points.back();
    }
    return m_first_point;
}

} // namespace vectorPath
