#include "vp_drafting_state.h"

#include <algorithm>
#include <cmath>

namespace Vp
{
namespace
{

constexpr double kPi = 3.14159265358979323846;

VpPoint2d unitVector(double angle_degrees)
{
    const double angle_radians = angle_degrees * kPi / 180.0;
    return {std::cos(angle_radians), std::sin(angle_radians)};
}

} // namespace

VpGridBasis draftingGridBasis(double rotation_degrees)
{
    return {unitVector(rotation_degrees), unitVector(rotation_degrees + 90.0)};
}

VpPoint2d snapPointToDraftingGrid(const VpPoint2d& point, double spacing, const VpGridBasis& basis)
{
    const double determinant =
        (basis.first_axis.x * basis.second_axis.y) - (basis.first_axis.y * basis.second_axis.x);
    if (!std::isfinite(spacing) || spacing <= 1.0e-9 || std::abs(determinant) <= 1.0e-9)
    {
        return point;
    }
    const double first_coordinate =
        ((point.x * basis.second_axis.y) - (point.y * basis.second_axis.x)) / determinant;
    const double second_coordinate =
        ((basis.first_axis.x * point.y) - (basis.first_axis.y * point.x)) / determinant;
    const double snapped_first = std::round(first_coordinate / spacing) * spacing;
    const double snapped_second = std::round(second_coordinate / spacing) * spacing;
    return {(basis.first_axis.x * snapped_first) + (basis.second_axis.x * snapped_second),
            (basis.first_axis.y * snapped_first) + (basis.second_axis.y * snapped_second)};
}

bool VpDraftingState::setGridSnapEnabled(bool is_enabled) noexcept
{
    if (m_is_grid_snap_enabled == is_enabled)
    {
        return false;
    }
    m_is_grid_snap_enabled = is_enabled;
    return true;
}

bool VpDraftingState::isGridSnapEnabled() const noexcept
{
    return m_is_grid_snap_enabled;
}

bool VpDraftingState::setGridSnapSpacing(double spacing) noexcept
{
    if (!std::isfinite(spacing) || spacing <= 1.0e-9 || spacing > 1.0e9)
    {
        return false;
    }
    m_grid_snap_spacing = spacing;
    return true;
}

double VpDraftingState::gridSnapSpacing() const noexcept
{
    return m_grid_snap_spacing;
}

bool VpDraftingState::setGridRotation(double rotation_degrees) noexcept
{
    if (!std::isfinite(rotation_degrees))
    {
        return false;
    }
    m_grid_rotation = std::fmod(rotation_degrees, 360.0);
    if (m_grid_rotation < 0.0)
    {
        m_grid_rotation += 360.0;
    }
    return true;
}

double VpDraftingState::gridRotation() const noexcept
{
    return m_grid_rotation;
}

bool VpDraftingState::setOrthoEnabled(bool is_enabled) noexcept
{
    if (m_is_ortho_enabled == is_enabled)
    {
        return false;
    }
    m_is_ortho_enabled = is_enabled;
    return true;
}

bool VpDraftingState::isOrthoEnabled() const noexcept
{
    return m_is_ortho_enabled;
}

bool VpDraftingState::setTrackingEnabled(bool is_enabled) noexcept
{
    if (m_is_tracking_enabled == is_enabled)
    {
        return false;
    }
    m_is_tracking_enabled = is_enabled;
    if (!is_enabled)
    {
        clearTrackingPoints();
    }
    return true;
}

bool VpDraftingState::isTrackingEnabled() const noexcept
{
    return m_is_tracking_enabled;
}

void VpDraftingState::acquireTrackingPoint(const VpPoint2d& point)
{
    const bool already_acquired = std::any_of(m_tracking_points.begin(), m_tracking_points.end(),
                                              [&](const VpPoint2d& anchor)
                                              {
                                                  return distance(anchor, point) <= 1.0e-9;
                                              });
    if (already_acquired)
    {
        return;
    }
    if (m_tracking_points.size() >= 8)
    {
        m_tracking_points.erase(m_tracking_points.begin());
    }
    m_tracking_points.push_back(point);
}

void VpDraftingState::clearTrackingPoints() noexcept
{
    m_tracking_points.clear();
}

const std::vector<VpPoint2d>& VpDraftingState::trackingPoints() const noexcept
{
    return m_tracking_points;
}

std::optional<VpPoint2d>
VpDraftingState::trackingSnap(const VpPoint2d& cursor,
                              const std::optional<VpPoint2d>& reference_point,
                              double tolerance) const
{
    if (!m_is_tracking_enabled)
    {
        return std::nullopt;
    }
    std::vector<VpPoint2d> anchors = m_tracking_points;
    if (reference_point && std::none_of(anchors.begin(), anchors.end(),
                                        [&](const VpPoint2d& point)
                                        {
                                            return distance(point, *reference_point) <= 1.0e-9;
                                        }))
    {
        anchors.push_back(*reference_point);
    }
    double best_distance = tolerance;
    std::optional<VpPoint2d> result;
    const auto consider = [&](const VpPoint2d& point)
    {
        const double candidate_distance = distance(cursor, point);
        if (candidate_distance <= best_distance)
        {
            best_distance = candidate_distance;
            result = point;
        }
    };
    for (const VpPoint2d& anchor : anchors)
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
    for (const VpPoint2d& vertical_anchor : anchors)
    {
        for (const VpPoint2d& horizontal_anchor : anchors)
        {
            consider({vertical_anchor.x, horizontal_anchor.y});
        }
    }
    return result;
}

VpPoint2d
VpDraftingState::constrainToGridAndOrtho(const VpPoint2d& point,
                                         const std::optional<VpPoint2d>& reference_point) const
{
    VpPoint2d result = point;
    if (m_is_grid_snap_enabled && m_grid_snap_spacing > 1.0e-9)
    {
        result = snapPointToDraftingGrid(result, m_grid_snap_spacing,
                                         draftingGridBasis(m_grid_rotation));
    }
    if (m_is_ortho_enabled && reference_point)
    {
        const double delta_x = std::abs(result.x - reference_point->x);
        const double delta_y = std::abs(result.y - reference_point->y);
        if (delta_x >= delta_y)
        {
            result.y = reference_point->y;
        }
        else
        {
            result.x = reference_point->x;
        }
    }
    return result;
}

} // namespace Vp
