#pragma once

#include "vp_bitmap_benchmark_types.h"

#include <vector>

namespace Vp
{

// 把基准结果写成报告文件。
VpResult<QString>
writeBitmapBenchmarkReport(const VpBitmapBenchmarkOptions& options,
                           const std::vector<VpBitmapBenchmarkCaseResult>& results,
                           quint64 peak_working_set_bytes);

} // namespace Vp
