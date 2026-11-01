#pragma once

#include "vp_geometry_types.h"

#include <array>
#include <vector>

namespace Vp
{

// 两点直线图元。
struct VpLineEntity
{
    VpPoint2d start_point;
    VpPoint2d end_point;
};

// 圆图元，仅由圆心与半径定义。
struct VpCircleEntity
{
    VpPoint2d center;
    double radius = 0.0;
};

// 圆弧图元：圆心、半径、起止角与方向。
struct VpArcEntity
{
    VpPoint2d center;
    double radius = 0.0;
    double start_angle = 0.0;
    double end_angle = 0.0;
    bool is_clockwise = false;
};

// 多段线图元：顶点、凸度与起止线宽。
struct VpPolylineEntity
{
    std::vector<VpPoint2d> vertices;
    bool is_closed = false;
    std::vector<double> bulges;
    std::vector<double> start_widths;
    std::vector<double> end_widths;
};

// 三次样条段，由四个控制点定义。
struct VpSplineEntity
{
    std::array<VpPoint2d, 4> control_points;
};

// 椭圆图元，用长轴与短轴向量描述姿态。
struct VpEllipseEntity
{
    VpPoint2d center;
    VpPoint2d major_axis;
    VpPoint2d minor_axis;
};

} // namespace Vp
