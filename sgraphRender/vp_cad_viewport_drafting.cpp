#include "vp_cad_viewport.h"

namespace Vp
{

void VpCadViewport::setGridSnapEnabled(bool is_enabled)
{
    if (m_drafting_state.setGridSnapEnabled(is_enabled))
    {
        emit gridSnapChanged(is_enabled);
    }
}

bool VpCadViewport::isGridSnapEnabled() const noexcept
{
    return m_drafting_state.isGridSnapEnabled();
}

void VpCadViewport::setGridSnapSpacing(double spacing)
{
    if (m_drafting_state.setGridSnapSpacing(spacing))
    {
        update();
    }
}

double VpCadViewport::gridSnapSpacing() const noexcept
{
    return m_drafting_state.gridSnapSpacing();
}

void VpCadViewport::setGridRotation(double rotation_degrees)
{
    if (m_drafting_state.setGridRotation(rotation_degrees))
    {
        update();
    }
}

double VpCadViewport::gridRotation() const noexcept
{
    return m_drafting_state.gridRotation();
}

} // namespace Vp
