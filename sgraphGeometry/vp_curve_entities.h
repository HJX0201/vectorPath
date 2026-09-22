#pragma once

#include "vp_geometry_types.h"

#include <array>
#include <vector>

namespace Vp
{

struct VpLineEntity
{
    VpPoint2d start_point;
    VpPoint2d end_point;
};

struct VpCircleEntity
{
    VpPoint2d center;
    double radius = 0.0;
};

struct VpArcEntity
{
    VpPoint2d center;
    double radius = 0.0;
    double start_angle = 0.0;
    double end_angle = 0.0;
    bool is_clockwise = false;
};

struct VpPolylineEntity
{
    std::vector<VpPoint2d> vertices;
    bool is_closed = false;
    std::vector<double> bulges;
    std::vector<double> start_widths;
    std::vector<double> end_widths;
};

struct VpSplineEntity
{
    std::array<VpPoint2d, 4> control_points;
};

struct VpEllipseEntity
{
    VpPoint2d center;
    VpPoint2d major_axis;
    VpPoint2d minor_axis;
};

} // namespace Vp
