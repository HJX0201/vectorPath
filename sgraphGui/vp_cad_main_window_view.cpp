#include "vp_cad_main_window.h"
#include "vp_cad_viewport.h"
#include "vp_cad_workspace_widget.h"
#include "vp_command_line_widget.h"

namespace Vp
{

bool VpCadMainWindow::executeViewCommand(const QString& normalized_command)
{
    VpCadViewport* viewport = m_workspace->viewport();
    if (normalized_command == QLatin1String("AUTOZOOM") ||
        normalized_command == QLatin1String("AZ") ||
        normalized_command == QLatin1String("ZOOM EXTENTS") ||
        normalized_command == QLatin1String("ZE"))
    {
        viewport->zoomExtents();
        return true;
    }
    return false;
}

} // namespace Vp
