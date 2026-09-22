#pragma once

#include "vp_geometry_types.h"

#include <optional>
#include <vector>

namespace Vp
{

struct VpGridBasis
{
    VpPoint2d first_axis;
    VpPoint2d second_axis;
};

VpGridBasis draftingGridBasis(double rotation_degrees);
VpPoint2d snapPointToDraftingGrid(const VpPoint2d& point, double spacing, const VpGridBasis& basis);

class VpDraftingState final
{
  public:
    bool setGridSnapEnabled(bool is_enabled) noexcept;
    bool isGridSnapEnabled() const noexcept;
    bool setGridSnapSpacing(double spacing) noexcept;
    double gridSnapSpacing() const noexcept;
    bool setGridRotation(double rotation_degrees) noexcept;
    double gridRotation() const noexcept;
    bool setOrthoEnabled(bool is_enabled) noexcept;
    bool isOrthoEnabled() const noexcept;
    bool setTrackingEnabled(bool is_enabled) noexcept;
    bool isTrackingEnabled() const noexcept;

    void acquireTrackingPoint(const VpPoint2d& point);
    void clearTrackingPoints() noexcept;
    const std::vector<VpPoint2d>& trackingPoints() const noexcept;
    std::optional<VpPoint2d> trackingSnap(const VpPoint2d& cursor,
                                          const std::optional<VpPoint2d>& reference_point,
                                          double tolerance) const;
    VpPoint2d constrainToGridAndOrtho(const VpPoint2d& point,
                                      const std::optional<VpPoint2d>& reference_point) const;

  private:
    std::vector<VpPoint2d> m_tracking_points;
    double m_grid_snap_spacing = 10.0;
    double m_grid_rotation = 0.0;
    bool m_is_grid_snap_enabled = false;
    bool m_is_ortho_enabled = false;
    bool m_is_tracking_enabled = true;
};

} // namespace Vp
