#pragma once

#include "vp_bitmap_benchmark_types.h"

#include <QImage>

namespace Vp
{

struct VpBitmapValidationResult
{
    bool passed = false;
    QString error;
    QByteArray flood_hash;
    QByteArray serial_hash;
    QByteArray parallel_hash;
    QImage difference;
};

VpBitmapValidationResult validateBitmapBenchmarkCase(const QImage& source,
                                                     const VpBitmapVectorResult& flood_fill,
                                                     const VpBitmapVectorResult& run_serial,
                                                     const VpBitmapVectorResult& run_parallel);

} // namespace Vp
