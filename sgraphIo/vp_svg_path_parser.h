#pragma once

#include "vp_result.h"

#include <QPainterPath>
#include <QString>
#include <vector>

namespace Vp
{

// SVG 路径解析出的一段子路径。
struct VpSvgSubpath
{
    QPainterPath path;
    bool is_closed = false;
};

// 解析 SVG path 数据为子路径。
VpResult<std::vector<VpSvgSubpath>> parseSvgPathData(const QString& path_data);

} // namespace Vp
