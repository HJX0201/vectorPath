#include "s_cad_document.h"
#include "s_cad_viewport.h"

#include <QKeyEvent>
#include <QKeySequence>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <algorithm>

namespace smartCam
{
namespace
{

constexpr double kMinimumZoom = 0.01;
constexpr double kMaximumZoom = 10000.0;

bool toolHasCursorPreview(SToolMode tool_mode)
{
    return tool_mode == SToolMode::Move || tool_mode == SToolMode::Copy ||
           tool_mode == SToolMode::Rotate || tool_mode == SToolMode::Scale ||
           tool_mode == SToolMode::Mirror || tool_mode == SToolMode::Trim ||
           tool_mode == SToolMode::Extend || tool_mode == SToolMode::Break ||
           tool_mode == SToolMode::Join || tool_mode == SToolMode::Explode ||
           tool_mode == SToolMode::Stretch || tool_mode == SToolMode::Lengthen ||
           tool_mode == SToolMode::Fillet || tool_mode == SToolMode::Chamfer ||
           tool_mode == SToolMode::Blend || tool_mode == SToolMode::Align ||
           tool_mode == SToolMode::ArrayRect || tool_mode == SToolMode::ArrayPolar ||
           tool_mode == SToolMode::ArrayPath || tool_mode == SToolMode::Offset ||
           tool_mode == SToolMode::Text || tool_mode == SToolMode::MText ||
           tool_mode == SToolMode::Leader || dimensionTypeForToolMode(tool_mode).has_value() ||
           tool_mode == SToolMode::Hatch;
}

} // namespace

void SCadViewport::initializeGL()
{
    initializeOpenGLFunctions();
    glDisable(GL_DEPTH_TEST);
}

void SCadViewport::paintGL()
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

void SCadViewport::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void SCadViewport::mousePressEvent(QMouseEvent* event)
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
        if (m_tool_mode == SToolMode::Select && !m_selected_entity_ids.empty())
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
        if (m_tool_mode == SToolMode::Polyline && m_input_points.size() >= 2)
        {
            completePolyline();
        }
        else if (m_tool_mode == SToolMode::PolylineEdit)
        {
            handlePolylineEditKeyword(QStringLiteral("DONE"));
        }
        else if (m_tool_mode == SToolMode::SplineEdit)
        {
            handleSplineEditKeyword(QStringLiteral("DONE"));
        }
        else if (m_tool_mode == SToolMode::Join)
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
        if (m_tool_mode == SToolMode::Select)
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

void SCadViewport::mouseMoveEvent(QMouseEvent* event)
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

void SCadViewport::mouseReleaseEvent(QMouseEvent* event)
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
        const SPoint2d release_world = screenToWorld(event->localPos());
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

void SCadViewport::wheelEvent(QWheelEvent* event)
{
    const QPointF event_position = event->posF();
    const SPoint2d world_before = screenToWorld(event_position);
    const double factor = event->angleDelta().y() > 0 ? 1.18 : (1.0 / 1.18);
    m_zoom = std::clamp(m_zoom * factor, kMinimumZoom, kMaximumZoom);
    const QPointF screen_after = worldToScreen(world_before);
    m_pan_offset += event_position - screen_after;
    update();
}

void SCadViewport::keyPressEvent(QKeyEvent* event)
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
        setOrthoEnabled(!m_is_ortho_enabled);
        emit commandMessage(m_is_ortho_enabled ? tr("正交 开") : tr("正交 关"));
        return;
    }
    if (event->key() == Qt::Key_F9)
    {
        setGridSnapEnabled(!m_is_grid_snap_enabled);
        emit commandMessage(m_is_grid_snap_enabled ? tr("捕捉模式 开") : tr("捕捉模式 关"));
        return;
    }
    if (event->key() == Qt::Key_F11)
    {
        setTrackingEnabled(!m_is_tracking_enabled);
        emit commandMessage(m_is_tracking_enabled ? tr("对象追踪 开") : tr("对象追踪 关"));
        return;
    }
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) &&
        m_tool_mode == SToolMode::Polyline)
    {
        completePolyline();
        return;
    }
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) &&
        m_tool_mode == SToolMode::Join)
    {
        completeJoin();
        return;
    }
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) &&
        m_tool_mode == SToolMode::PolylineEdit)
    {
        handlePolylineEditKeyword(QStringLiteral("DONE"));
        return;
    }
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) &&
        m_tool_mode == SToolMode::SplineEdit)
    {
        handleSplineEditKeyword(QStringLiteral("DONE"));
        return;
    }
    if ((event->key() == Qt::Key_Space || event->key() == Qt::Key_Return ||
         event->key() == Qt::Key_Enter) &&
        m_tool_mode == SToolMode::Select)
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
        if (m_tool_mode == SToolMode::Select && !m_selected_entity_ids.empty())
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

} // namespace smartCam
