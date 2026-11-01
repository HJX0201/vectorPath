#pragma once

#include "vp_bitmap_vectorizer.h"

#include <QString>
#include <QtGlobal>
#include <vector>

namespace Vp
{

// 一个位图矢量化基准测试用例。
struct VpBitmapBenchmarkCase
{
    int id = 0;
    QString category;
    int width = 0;
    int height = 0;
    quint32 seed = 0;
    int color_count = 0;
    QString file_path;
};

// 单个算法在一次基准中的耗时与结果摘要。
struct VpBitmapAlgorithmSummary
{
    bool success = false;
    QString error;
    double milliseconds = 0.0;
    QByteArray contour_hash;
    VpBitmapVectorMetrics metrics;
};

// 一个用例下三种算法的对比结果。
struct VpBitmapBenchmarkCaseResult
{
    VpBitmapBenchmarkCase test_case;
    VpBitmapAlgorithmSummary flood_fill;
    VpBitmapAlgorithmSummary run_serial;
    VpBitmapAlgorithmSummary run_parallel;
    bool passed = false;
    QString validation_error;
};

// 基准测试的用例数量、线程与输出设置。
struct VpBitmapBenchmarkOptions
{
    int case_count = 1000;
    quint32 seed = 20260727U;
    int thread_count = 0;
    int repetitions = 3;
    bool smoke = false;
    QString output_directory;
};

} // namespace Vp
