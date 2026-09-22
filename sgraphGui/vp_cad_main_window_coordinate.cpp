#include "vp_cad_main_window.h"
#include "vp_cad_viewport.h"
#include "vp_cad_workspace_widget.h"
#include "vp_command_line_widget.h"
#include "vp_coordinate_input.h"

namespace Vp
{

bool VpCadMainWindow::executeCoordinateInput(const QString& command)
{
    const VpCoordinateInput input = parseCoordinateInput(command);
    if (input.mode == VpCoordinateInputMode::NotCoordinate)
    {
        return false;
    }
    if (input.mode == VpCoordinateInputMode::Invalid)
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

void VpCadMainWindow::previewCommandInput(const QString& command)
{
    const VpCoordinateInput input = parseCoordinateInput(command);
    if (input.mode == VpCoordinateInputMode::AbsoluteCartesian ||
        input.mode == VpCoordinateInputMode::RelativeCartesian ||
        input.mode == VpCoordinateInputMode::RelativePolar)
    {
        m_workspace->viewport()->previewCoordinateInput(input);
        return;
    }
    m_workspace->viewport()->clearCoordinateInputPreview();
}

} // namespace Vp
