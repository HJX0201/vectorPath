#include "s_cad_main_window.h"
#include "s_cad_viewport.h"
#include "s_cad_workspace_widget.h"
#include "s_command_line_widget.h"

namespace smartGraphics
{

bool SCadMainWindow::executeViewCommand(const QString& normalized_command)
{
    SCadViewport* viewport = m_workspace->viewport();
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

} // namespace smartGraphics
