#pragma once

#include "vp_bitmap_benchmark_types.h"
#include "vp_result.h"

#include <vector>

namespace Vp
{

VpResult<std::vector<VpBitmapBenchmarkCase>>
generateBitmapBenchmarkCases(const VpBitmapBenchmarkOptions& options);

} // namespace Vp
