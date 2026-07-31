#pragma once

#include "s_result.h"

#include <QByteArray>
#include <QColor>
#include <QImage>
#include <QPoint>
#include <QtGlobal>
#include <vector>

namespace vectorPath
{

struct SBitmapVectorSettings
{
    bool ignore_background = true;
    QColor background_color;
    int worker_count = 0;
};

struct SBitmapVectorMetrics
{
    qint64 pixel_count = 0;
    qint64 run_count = 0;
    qint64 component_count = 0;
    qint64 segment_count = 0;
    qint64 contour_count = 0;
    qint64 scan_nanoseconds = 0;
    qint64 connection_nanoseconds = 0;
    qint64 stitch_nanoseconds = 0;
    qint64 total_nanoseconds = 0;
    qint64 estimated_working_bytes = 0;
    qint64 svg_bytes = 0;
    int worker_count = 1;
};

struct SBitmapContour
{
    QRgb color = 0;
    int block_id = -1;
    std::vector<QPoint> points;
};

struct SBitmapVectorResult
{
    QByteArray svg_data;
    std::vector<SBitmapContour> contours;
    SBitmapVectorMetrics metrics;
};

SResult<SBitmapVectorResult> bitmapToVectorResult(
    const QImage& source, const SBitmapVectorSettings& settings);

QByteArray bitmapContoursToSvgData(
    int width, int height, const std::vector<SBitmapContour>& contours);

SResult<QByteArray> bitmapToSvgData(const QImage& source,
                                    const SBitmapVectorSettings& settings);

} // namespace vectorPath
