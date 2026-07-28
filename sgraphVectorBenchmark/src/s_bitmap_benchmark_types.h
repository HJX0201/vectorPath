#pragma once

#include "s_bitmap_vectorizer.h"

#include <QString>
#include <QtGlobal>
#include <vector>

namespace smartGraphics
{

struct SBitmapBenchmarkCase
{
    int id = 0;
    QString category;
    int width = 0;
    int height = 0;
    quint32 seed = 0;
    int color_count = 0;
    QString file_path;
};

struct SBitmapAlgorithmSummary
{
    bool success = false;
    QString error;
    double milliseconds = 0.0;
    QByteArray contour_hash;
    SBitmapVectorMetrics metrics;
};

struct SBitmapBenchmarkCaseResult
{
    SBitmapBenchmarkCase test_case;
    SBitmapAlgorithmSummary flood_fill;
    SBitmapAlgorithmSummary run_serial;
    SBitmapAlgorithmSummary run_parallel;
    bool passed = false;
    QString validation_error;
};

struct SBitmapBenchmarkOptions
{
    int case_count = 1000;
    quint32 seed = 20260727U;
    int thread_count = 0;
    int repetitions = 3;
    bool smoke = false;
    QString output_directory;
};

} // namespace smartGraphics
