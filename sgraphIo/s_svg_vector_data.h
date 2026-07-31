#pragma once

#include "s_geometry_types.h"

#include <QColor>
#include <QStringList>
#include <vector>

namespace smartCam
{

enum class SVectorFillRule
{
    EvenOdd,
    NonZero
};

struct SVectorOutline
{
    std::vector<SPoint2d> points;
    bool is_closed = false;
    QColor color{0, 0, 0};
};

struct SVectorRegion
{
    std::vector<std::vector<SPoint2d>> contours;
    QColor color{0, 0, 0};
    SVectorFillRule fill_rule = SVectorFillRule::NonZero;
};

struct SSvgVectorData
{
    std::vector<SVectorOutline> outlines;
    std::vector<SVectorRegion> regions;
    double source_width = 0.0;
    double source_height = 0.0;
    QStringList warnings;
};

} // namespace smartCam
