#pragma once

#include "s_bitmap_vectorizer.h"

namespace smartGraphics
{

SResult<SBitmapVectorResult> bitmapToVectorFloodFill(
    const QImage& source, const SBitmapVectorSettings& settings);

} // namespace smartGraphics
