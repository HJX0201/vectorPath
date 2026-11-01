#pragma once

#include "vp_geometry_types.h"

#include <optional>
#include <vector>

namespace Vp
{

// 旋转栅格的两条基向量，用于捕捉坐标换算。
struct VpGridBasis
{
    VpPoint2d first_axis;
    VpPoint2d second_axis;
};

// 按旋转角构造栅格基向量。
VpGridBasis draftingGridBasis(double rotation_degrees);
// 把点吸附到栅格交点。
VpPoint2d snapPointToDraftingGrid(const VpPoint2d& point, double spacing, const VpGridBasis& basis);

// 绘图辅助状态：正交、栅格捕捉与极轴追踪。
class VpDraftingState final
{
  public:
    // 启用或关闭栅格捕捉。
    bool setGridSnapEnabled(bool is_enabled) noexcept;
    // 返回栅格捕捉是否启用。
    bool isGridSnapEnabled() const noexcept;
    // 设置栅格捕捉间距，非法值返回 false。
    bool setGridSnapSpacing(double spacing) noexcept;
    // 返回栅格捕捉间距。
    double gridSnapSpacing() const noexcept;
    // 设置栅格旋转角度，非法值返回 false。
    bool setGridRotation(double rotation_degrees) noexcept;
    // 返回栅格旋转角度。
    double gridRotation() const noexcept;
    // 启用或关闭正交模式。
    bool setOrthoEnabled(bool is_enabled) noexcept;
    // 返回正交模式是否启用。
    bool isOrthoEnabled() const noexcept;
    // 启用或关闭极轴追踪。
    bool setTrackingEnabled(bool is_enabled) noexcept;
    // 返回极轴追踪是否启用。
    bool isTrackingEnabled() const noexcept;

    // 记录一个追踪点，供极轴对齐使用。
    void acquireTrackingPoint(const VpPoint2d& point);
    // 清空已记录的追踪点。
    void clearTrackingPoints() noexcept;
    // 返回当前追踪点集合。
    const std::vector<VpPoint2d>& trackingPoints() const noexcept;
    // 在追踪点上做极轴捕捉。
    std::optional<VpPoint2d> trackingSnap(const VpPoint2d& cursor,
                                          const std::optional<VpPoint2d>& reference_point,
                                          double tolerance) const;
    // 把点约束到栅格与正交方向。
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
