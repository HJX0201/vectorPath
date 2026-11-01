#pragma once

#include "vp_bitmap_benchmark_types.h"

#include <QImage>

namespace Vp
{

// 三种算法结果一致性校验的结论。
struct VpBitmapValidationResult
{
    bool passed = false;
    QString error;
    QByteArray flood_hash;
    QByteArray serial_hash;
    QByteArray parallel_hash;
    QImage difference;
};

// 校验三种算法的结果是否一致。
VpBitmapValidationResult validateBitmapBenchmarkCase(const QImage& source,
                                                     const VpBitmapVectorResult& flood_fill,
                                                     const VpBitmapVectorResult& run_serial,
                                                     const VpBitmapVectorResult& run_parallel);

} // namespace Vp
