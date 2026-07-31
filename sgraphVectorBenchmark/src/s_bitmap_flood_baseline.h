#pragma once

#include "s_bitmap_vectorizer.h"

namespace smartCam
{

SResult<SBitmapVectorResult> bitmapToVectorFloodFill(
    const QImage& source, const SBitmapVectorSettings& settings);

} // namespace smartCam
