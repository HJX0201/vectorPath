#include "s_cad_viewport.h"

#include <algorithm>
#include <limits>

namespace smartCam
{

void SCadViewport::setTrackingEnabled(bool is_enabled)
{
    if (m_is_tracking_enabled == is_enabled)
    {
        return;
    }
    m_is_tracking_enabled = is_enabled;
    if (!is_enabled)
    {
        m_tracking_points.clear();
    }
    emit trackingChanged(is_enabled);
    update();
}

bool SCadViewport::isTrackingEnabled() const noexcept
{
    return m_is_tracking_enabled;
}

std::optional<SObjectSnapResult>
SCadViewport::trackingSnap(const SPoint2d& cursor, const std::optional<SPoint2d>& reference_point,
                           double tolerance) const
{
    if (!m_is_tracking_enabled)
    {
        return std::nullopt;
    }
    std::vector<SPoint2d> anchors = m_tracking_points;
    if (reference_point && std::none_of(anchors.begin(), anchors.end(),
                                        [&](const SPoint2d& point)
                                        {
                                            return distance(point, *reference_point) <= 1.0e-9;
                                        }))
    {
        anchors.push_back(*reference_point);
    }
    double best_distance = tolerance;
    std::optional<SObjectSnapResult> result;
    const auto consider = [&](const SPoint2d& point)
    {
        const double candidate_distance = distance(cursor, point);
        if (candidate_distance <= best_distance)
        {
            best_distance = candidate_distance;
            result = SObjectSnapResult{point, SObjectSnapType::Tracking};
        }
    };
    for (const SPoint2d& anchor : anchors)
    {
        if (std::abs(cursor.x - anchor.x) <= tolerance)
        {
            consider({anchor.x, cursor.y});
        }
        if (std::abs(cursor.y - anchor.y) <= tolerance)
        {
            consider({cursor.x, anchor.y});
        }
    }
    for (const SPoint2d& vertical_anchor : anchors)
    {
        for (const SPoint2d& horizontal_anchor : anchors)
        {
            consider({vertical_anchor.x, horizontal_anchor.y});
        }
    }
    return result;
}

} // namespace smartCam
