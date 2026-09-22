#pragma once

#include "vp_geometry_types.h"

#include <QColor>
#include <QStringList>
#include <vector>

namespace Vp
{

enum class VpVectorFillRule
{
    EvenOdd,
    NonZero
};

struct VpVectorOutline
{
    std::vector<VpPoint2d> points;
    bool is_closed = false;
    QColor color{0, 0, 0};
};

struct VpVectorRegion
{
    std::vector<std::vector<VpPoint2d>> contours;
    QColor color{0, 0, 0};
    VpVectorFillRule fill_rule = VpVectorFillRule::NonZero;
};

struct VpSvgVectorData
{
    std::vector<VpVectorOutline> outlines;
    std::vector<VpVectorRegion> regions;
    double source_width = 0.0;
    double source_height = 0.0;
    QStringList warnings;
};

} // namespace Vp
