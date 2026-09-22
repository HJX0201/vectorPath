#include "vp_cad_document.h"
#include "vp_cad_viewport.h"

#include <QKeyEvent>
#include <QKeySequence>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <algorithm>

namespace Vp
{
namespace
{

constexpr double kMinimumZoom = 0.01;
constexpr double kMaximumZoom = 10000.0;

bool toolHasCursorPreview(VpToolMode tool_mode)
{
    return tool_mode == VpToolMode::Move || tool_mode == VpToolMode::Copy ||
           tool_mode == VpToolMode::Rotate || tool_mode == VpToolMode::Scale ||
           tool_mode == VpToolMode::Mirror || tool_mode == VpToolMode::Trim ||
           tool_mode == VpToolMode::Extend || tool_mode == VpToolMode::Break ||
           tool_mode == VpToolMode::Join || tool_mode == VpToolMode::Explode ||
           tool_mode == VpToolMode::Stretch || tool_mode == VpToolMode::Lengthen ||
           tool_mode == VpToolMode::Fillet || tool_mode == VpToolMode::Chamfer ||
           tool_mode == VpToolMode::Blend || tool_mode == VpToolMode::Align ||
           tool_mode == VpToolMode::ArrayRect || tool_mode == VpToolMode::ArrayPolar ||
           tool_mode == VpToolMode::ArrayPath || tool_mode == VpToolMode::Offset ||
           tool_mode == VpToolMode::Text || tool_mode == VpToolMode::MText ||
           tool_mode == VpToolMode::Leader || dimensionTypeForToolMode(tool_mode).has_value() ||
           tool_mode == VpToolMode::Hatch;
}

} // namespace

void VpCadViewport::initializeGL()
{
    initializeOpenGLFunctions();
    glDisable(GL_DEPTH_TEST);
}

void VpCadViewport::paintGL()
{
    glClearColor(static_cast<GLfloat>(m_canvas_color.redF()),
                 static_cast<GLfloat>(m_canvas_color.greenF()),
                 static_cast<GLfloat>(m_canvas_color.blueF()), 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    if (m_is_grid_visible)
    {
        drawGrid(painter);
    }
    drawEntities(painter);
    drawEntityDisplayOverlay(painter);
    drawSimulationOverlay(painter);
    drawPreview(painter);
    drawGrips(painter);
    drawNavigationOverlay(painter);
}

void VpCadViewport::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void VpCadViewport::mousePressEvent(QMouseEvent* event)
{
    setFocus();
    m_last_mouse_position = event->pos();
    if (event->button() == Qt::MiddleButton)
    {
        m_is_panning = true;
        setCursor(Qt::ClosedHandCursor);
        return;
    }
    if (event->button() == Qt::RightButton)
    {
        if (m_tool_mode == VpToolMode::Select && !m_selected_entity_ids.empty())
        {
            emit selectionContextRequested(event->globalPos());
            return;
        }
        if (m_is_grip_dragging)
        {
            cancelGripDrag();
            emit commandMessage(tr("*夹点编辑已取消*"));
            return;
        }
        if (m_tool_mode == VpToolMode::Polyline && m_input_points.size() >= 2)
        {
            completePolyline();
        }
        else if (m_tool_mode == VpToolMode::PolylineEdit)
        {
            handlePolylineEditKeyword(QStringLiteral("DONE"));
        }
        else if (m_tool_mode == VpToolMode::SplineEdit)
        {
            handleSplineEditKeyword(QStringLiteral("DONE"));
        }
        else if (m_tool_mode == VpToolMode::Join)
        {
            completeJoin();
        }
        else
        {
            cancelCommand();
        }
        return;
    }
    if (event->button() == Qt::LeftButton)
    {
        if (m_tool_mode == VpToolMode::Select)
        {
            if (beginGripDrag(event->localPos()))
            {
                return;
            }
            m_selection_start_screen = event->pos();
            m_selection_current_screen = event->pos();
            m_selection_start_world = screenToWorld(event->localPos());
            m_is_additive_selection = event->modifiers().testFlag(Qt::ShiftModifier);
            m_is_box_selecting = true;
            update();
            return;
        }
        acceptPoint(constrainedPoint(screenToWorld(event->localPos())));
    }
}

void VpCadViewport::mouseMoveEvent(QMouseEvent* event)
{
    if (m_is_panning)
    {
        const QPoint delta = event->pos() - m_last_mouse_position;
        m_pan_offset += QPointF(delta.x(), delta.y());
        m_last_mouse_position = event->pos();
        update();
    }
    m_pointer_world = constrainedPoint(screenToWorld(event->localPos()));
    if (!m_command_preview_point)
    {
        m_cursor_world = m_pointer_world;
    }
    emit cursorWorldPositionChanged(m_cursor_world);
    if (m_is_grip_dragging)
    {
        updateGripDrag(m_cursor_world);
        return;
    }
    if (m_is_box_selecting)
    {
        m_selection_current_screen = event->pos();
        update();
    }
    if (m_first_point.has_value() || !m_input_points.empty() || toolHasCursorPreview(m_tool_mode))
    {
        update();
    }
}

void VpCadViewport::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton)
    {
        m_is_panning = false;
        setCursor(Qt::CrossCursor);
    }
    if (event->button() == Qt::LeftButton && m_is_grip_dragging)
    {
        updateGripDrag(constrainedPoint(screenToWorld(event->localPos())));
        commitGripDrag();
        return;
    }
    if (event->button() == Qt::LeftButton && m_is_box_selecting)
    {
        m_selection_current_screen = event->pos();
        const QPoint drag_delta = m_selection_current_screen - m_selection_start_screen;
        const VpPoint2d release_world = screenToWorld(event->localPos());
        if (drag_delta.manhattanLength() < 5)
        {
            selectAt(release_world);
        }
        else
        {
            selectInWindow(m_selection_start_world, release_world,
                           m_selection_current_screen.x() < m_selection_start_screen.x());
        }
        m_is_box_selecting = false;
        update();
    }
}

void VpCadViewport::wheelEvent(QWheelEvent* event)
{
    const QPointF event_position = event->posF();
    const VpPoint2d world_before = screenToWorld(event_position);
    const double factor = event->angleDelta().y() > 0 ? 1.18 : (1.0 / 1.18);
    m_zoom = std::clamp(m_zoom * factor, kMinimumZoom, kMaximumZoom);
    const QPointF screen_after = worldToScreen(world_before);
    m_pan_offset += event_position - screen_after;
    update();
}

void VpCadViewport::keyPressEvent(QKeyEvent* event)
{
    if (m_is_grip_dragging && event->key() == Qt::Key_Space)
    {
        cycleGripOperation();
        return;
    }
    if (event->matches(QKeySequence::SelectAll))
    {
        selectAll();
        return;
    }
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
    {
        deleteSelected();
        return;
    }
    if (event->matches(QKeySequence::Undo))
    {
        if (m_document)
        {
            m_document->undo();
        }
        return;
    }
    if (event->matches(QKeySequence::Redo))
    {
        if (m_document)
        {
            m_document->redo();
        }
        return;
    }
    if (event->key() == Qt::Key_F3)
    {
        setObjectSnapEnabled(!m_is_object_snap_enabled);
        emit commandMessage(m_is_object_snap_enabled ? tr("对象捕捉 开") : tr("对象捕捉 关"));
        return;
    }
    if (event->key() == Qt::Key_F7)
    {
        setGridVisible(!m_is_grid_visible);
        emit commandMessage(m_is_grid_visible ? tr("栅格 开") : tr("栅格 关"));
        return;
    }
    if (event->key() == Qt::Key_F8)
    {
        setOrthoEnabled(!isOrthoEnabled());
        emit commandMessage(isOrthoEnabled() ? tr("正交 开") : tr("正交 关"));
        return;
    }
    if (event->key() == Qt::Key_F9)
    {
        setGridSnapEnabled(!isGridSnapEnabled());
        emit commandMessage(isGridSnapEnabled() ? tr("捕捉模式 开") : tr("捕捉模式 关"));
        return;
    }
    if (event->key() == Qt::Key_F11)
    {
        setTrackingEnabled(!isTrackingEnabled());
        emit commandMessage(isTrackingEnabled() ? tr("对象追踪 开") : tr("对象追踪 关"));
        return;
    }
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) &&
        m_tool_mode == VpToolMode::Polyline)
    {
        completePolyline();
        return;
    }
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) &&
        m_tool_mode == VpToolMode::Join)
    {
        completeJoin();
        return;
    }
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) &&
        m_tool_mode == VpToolMode::PolylineEdit)
    {
        handlePolylineEditKeyword(QStringLiteral("DONE"));
        return;
    }
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) &&
        m_tool_mode == VpToolMode::SplineEdit)
    {
        handleSplineEditKeyword(QStringLiteral("DONE"));
        return;
    }
    if ((event->key() == Qt::Key_Space || event->key() == Qt::Key_Return ||
         event->key() == Qt::Key_Enter) &&
        m_tool_mode == VpToolMode::Select)
    {
        setToolMode(m_last_tool_mode);
        return;
    }
    if (event->key() == Qt::Key_Escape)
    {
        if (m_is_grip_dragging)
        {
            cancelGripDrag();
            emit commandMessage(tr("*夹点编辑已取消*"));
            return;
        }
        if (m_tool_mode == VpToolMode::Select && !m_selected_entity_ids.empty())
        {
            clearSelection();
            emit commandMessage(tr("已清除选择集。"));
            return;
        }
        cancelCommand();
        return;
    }
    QOpenGLWidget::keyPressEvent(event);
}

} // namespace Vp
