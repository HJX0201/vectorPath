#pragma once

#include "vp_result.h"

#include <QByteArray>
#include <QColor>
#include <QImage>
#include <QPoint>
#include <QtGlobal>
#include <vector>

namespace Vp
{

// 位图矢量化参数：背景忽略与线程数。
struct VpBitmapVectorSettings
{
    bool ignore_background = true;
    QColor background_color;
    int worker_count = 0;
};

// 位图矢量化的耗时与规模统计。
struct VpBitmapVectorMetrics
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

// 矢量化得到的一条同色轮廓。
struct VpBitmapContour
{
    QRgb color = 0;
    int block_id = -1;
    std::vector<QPoint> points;
};

// 一次位图矢量化的产物：SVG 数据、轮廓与统计。
struct VpBitmapVectorResult
{
    QByteArray svg_data;
    std::vector<VpBitmapContour> contours;
    VpBitmapVectorMetrics metrics;
};

// 把位图转换为轮廓、SVG 与统计结果。
VpResult<VpBitmapVectorResult> bitmapToVectorResult(const QImage& source,
                                                    const VpBitmapVectorSettings& settings);

// 把轮廓序列化为 SVG 路径数据。
QByteArray bitmapContoursToSvgData(int width, int height,
                                   const std::vector<VpBitmapContour>& contours);

// 把位图直接转换为 SVG 字节流。
VpResult<QByteArray> bitmapToSvgData(const QImage& source, const VpBitmapVectorSettings& settings);

} // namespace Vp
