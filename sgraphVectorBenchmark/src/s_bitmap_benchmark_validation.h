#pragma once

#include "s_bitmap_benchmark_types.h"

#include <QImage>

namespace smartCam
{

struct SBitmapValidationResult
{
    bool passed = false;
    QString error;
    QByteArray flood_hash;
    QByteArray serial_hash;
    QByteArray parallel_hash;
    QImage difference;
};

SBitmapValidationResult validateBitmapBenchmarkCase(
    const QImage& source, const SBitmapVectorResult& flood_fill,
    const SBitmapVectorResult& run_serial,
    const SBitmapVectorResult& run_parallel);

} // namespace smartCam
