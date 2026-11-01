#pragma once

#include "vp_bitmap_vectorizer.h"

#include <QPoint>
#include <vector>

namespace Vp
{
namespace bitmapVectorPrivate
{

// 位图矢量化中间结果中的一段边界线段。
struct VpBoundarySegment
{
    QPoint start;
    QPoint end;
    QRgb color = 0;
    int owner_id = -1;
    int block_id = -1;
};

VpResult<std::vector<VpBoundarySegment>>
compressSegments(const std::vector<VpBoundarySegment>& source);

VpResult<std::vector<VpBitmapContour>> stitchSegments(std::vector<VpBoundarySegment>& segments);

} // namespace bitmapVectorPrivate
} // namespace Vp
