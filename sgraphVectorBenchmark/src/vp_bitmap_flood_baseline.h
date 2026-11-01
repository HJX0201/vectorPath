#pragma once

#include "vp_bitmap_vectorizer.h"

namespace Vp
{

// 用洪水填充基线算法把位图矢量化为 SVG。
VpResult<VpBitmapVectorResult> bitmapToVectorFloodFill(const QImage& source,
                                                       const VpBitmapVectorSettings& settings);

} // namespace Vp
