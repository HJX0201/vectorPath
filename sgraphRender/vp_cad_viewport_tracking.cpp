#include "vp_cad_viewport.h"

namespace Vp
{

void VpCadViewport::setTrackingEnabled(bool is_enabled)
{
    if (!m_drafting_state.setTrackingEnabled(is_enabled))
    {
        return;
    }
    emit trackingChanged(is_enabled);
    update();
}

bool VpCadViewport::isTrackingEnabled() const noexcept
{
    return m_drafting_state.isTrackingEnabled();
}

std::optional<VpObjectSnapResult>
VpCadViewport::trackingSnap(const VpPoint2d& cursor,
                            const std::optional<VpPoint2d>& reference_point, double tolerance) const
{
    const std::optional<VpPoint2d> point =
        m_drafting_state.trackingSnap(cursor, reference_point, tolerance);
    if (!point)
    {
        return std::nullopt;
    }
    return VpObjectSnapResult{*point, VpObjectSnapType::Tracking};
}

} // namespace Vp
