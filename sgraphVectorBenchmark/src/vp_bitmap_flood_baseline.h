#pragma once

#include "vp_bitmap_vectorizer.h"

namespace Vp
{

VpResult<VpBitmapVectorResult> bitmapToVectorFloodFill(const QImage& source,
                                                       const VpBitmapVectorSettings& settings);

} // namespace Vp
