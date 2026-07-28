#pragma once

#include "s_bitmap_vectorizer.h"

#include <QPoint>
#include <vector>

namespace smartGraphics
{
namespace bitmapVectorPrivate
{

struct SBoundarySegment
{
    QPoint start;
    QPoint end;
    QRgb color = 0;
    int owner_id = -1;
    int block_id = -1;
};

SResult<std::vector<SBoundarySegment>> compressSegments(
    const std::vector<SBoundarySegment>& source);

SResult<std::vector<SBitmapContour>> stitchSegments(
    std::vector<SBoundarySegment>& segments);

} // namespace bitmapVectorPrivate
} // namespace smartGraphics
