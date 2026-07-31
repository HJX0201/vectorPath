#pragma once

#include "s_bitmap_benchmark_types.h"
#include "s_result.h"

#include <vector>

namespace vectorPath
{

SResult<std::vector<SBitmapBenchmarkCase>> generateBitmapBenchmarkCases(
    const SBitmapBenchmarkOptions& options);

} // namespace vectorPath
