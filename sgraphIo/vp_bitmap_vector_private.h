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

// 合并共线的边界线段，压缩中间结果。
VpResult<std::vector<VpBoundarySegment>>
compressSegments(const std::vector<VpBoundarySegment>& source);

// 把边界线段拼接成闭合轮廓。
VpResult<std::vector<VpBitmapContour>> stitchSegments(std::vector<VpBoundarySegment>& segments);

} // namespace bitmapVectorPrivate
} // namespace Vp
