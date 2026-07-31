#pragma once

#include "s_bitmap_vectorizer.h"

namespace vectorPath
{

SResult<SBitmapVectorResult> bitmapToVectorFloodFill(
    const QImage& source, const SBitmapVectorSettings& settings);

} // namespace vectorPath
