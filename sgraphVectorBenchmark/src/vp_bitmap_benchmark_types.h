#pragma once

#include "vp_bitmap_vectorizer.h"

#include <QString>
#include <QtGlobal>
#include <vector>

namespace Vp
{

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

struct VpBitmapAlgorithmSummary
{
    bool success = false;
    QString error;
    double milliseconds = 0.0;
    QByteArray contour_hash;
    VpBitmapVectorMetrics metrics;
};

struct VpBitmapBenchmarkCaseResult
{
    VpBitmapBenchmarkCase test_case;
    VpBitmapAlgorithmSummary flood_fill;
    VpBitmapAlgorithmSummary run_serial;
    VpBitmapAlgorithmSummary run_parallel;
    bool passed = false;
    QString validation_error;
};

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
