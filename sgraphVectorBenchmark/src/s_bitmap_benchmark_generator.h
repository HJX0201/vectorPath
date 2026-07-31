#pragma once

#include "s_bitmap_benchmark_types.h"
#include "s_result.h"

#include <vector>

namespace smartCam
{

SResult<std::vector<SBitmapBenchmarkCase>> generateBitmapBenchmarkCases(
    const SBitmapBenchmarkOptions& options);

} // namespace smartCam
