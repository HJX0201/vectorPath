#pragma once

#include "s_bitmap_benchmark_types.h"

#include <vector>

namespace vectorPath
{

SResult<QString> writeBitmapBenchmarkReport(
    const SBitmapBenchmarkOptions& options,
    const std::vector<SBitmapBenchmarkCaseResult>& results,
    quint64 peak_working_set_bytes);

} // namespace vectorPath
