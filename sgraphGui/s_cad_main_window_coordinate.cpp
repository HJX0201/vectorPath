#include "s_cad_main_window.h"
#include "s_cad_viewport.h"
#include "s_cad_workspace_widget.h"
#include "s_command_line_widget.h"
#include "s_coordinate_input.h"

namespace smartCam
{

bool SCadMainWindow::executeCoordinateInput(const QString& command)
{
    const SCoordinateInput input = parseCoordinateInput(command);
    if (input.mode == SCoordinateInputMode::NotCoordinate)
    {
        return false;
    }
    if (input.mode == SCoordinateInputMode::Invalid)
    {
        m_command_line->appendMessage(tr("坐标格式无效。可输入 X,Y、@dx,dy 或 @距离<角度。"));
        return true;
    }
    if (!m_workspace->viewport()->submitCoordinateInput(input))
    {
        m_command_line->appendMessage(tr("相对坐标需要先指定一个参考点。"));
    }
    return true;
}

void SCadMainWindow::previewCommandInput(const QString& command)
{
    const SCoordinateInput input = parseCoordinateInput(command);
    if (input.mode == SCoordinateInputMode::AbsoluteCartesian ||
        input.mode == SCoordinateInputMode::RelativeCartesian ||
        input.mode == SCoordinateInputMode::RelativePolar)
    {
        m_workspace->viewport()->previewCoordinateInput(input);
        return;
    }
    m_workspace->viewport()->clearCoordinateInputPreview();
}

} // namespace smartCam
