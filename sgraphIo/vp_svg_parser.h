#pragma once

#include "vp_result.h"
#include "vp_svg_vector_data.h"

#include <QByteArray>

namespace Vp
{

// 解析 SVG 文本为矢量数据。
VpResult<VpSvgVectorData> parseSvgVectorData(const QByteArray& svg_data);

} // namespace Vp
