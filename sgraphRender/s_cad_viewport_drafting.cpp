#include "s_cad_viewport.h"

#include <QPainter>
#include <algorithm>
#include <cmath>

namespace smartCam
{
namespace
{

constexpr double kPi = 3.14159265358979323846;

SPoint2d unitVector(double angle_degrees)
{
    const double angle_radians = angle_degrees * kPi / 180.0;
    return {std::cos(angle_radians), std::sin(angle_radians)};
}

} // namespace

SGridBasis draftingGridBasis(double rotation_degrees)
{
    return {unitVector(rotation_degrees), unitVector(rotation_degrees + 90.0)};
}

SPoint2d snapPointToDraftingGrid(const SPoint2d& point, double spacing, const SGridBasis& basis)
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

void SCadViewport::setGridSnapEnabled(bool is_enabled)
{
    if (m_is_grid_snap_enabled == is_enabled)
    {
        return;
    }
    m_is_grid_snap_enabled = is_enabled;
    emit gridSnapChanged(m_is_grid_snap_enabled);
}

bool SCadViewport::isGridSnapEnabled() const noexcept
{
    return m_is_grid_snap_enabled;
}

void SCadViewport::setGridSnapSpacing(double spacing)
{
    if (!std::isfinite(spacing) || spacing <= 1.0e-9 || spacing > 1.0e9)
    {
        return;
    }
    m_grid_snap_spacing = spacing;
    update();
}

double SCadViewport::gridSnapSpacing() const noexcept
{
    return m_grid_snap_spacing;
}

void SCadViewport::setGridRotation(double rotation_degrees)
{
    if (!std::isfinite(rotation_degrees))
    {
        return;
    }
    m_grid_rotation = std::fmod(rotation_degrees, 360.0);
    if (m_grid_rotation < 0.0)
    {
        m_grid_rotation += 360.0;
    }
    update();
}

double SCadViewport::gridRotation() const noexcept
{
    return m_grid_rotation;
}

} // namespace smartCam
