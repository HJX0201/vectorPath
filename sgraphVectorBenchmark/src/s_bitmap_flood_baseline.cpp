#include "s_bitmap_flood_baseline.h"

#include "s_bitmap_vector_private.h"

#include <QElapsedTimer>
#include <QSet>
#include <vector>

namespace smartGraphics
{
namespace
{

QRgb effectiveColor(QRgb color, QRgb background,
                    const SBitmapVectorSettings& settings)
{
    if (qAlpha(color) == 0 ||
        (settings.ignore_background && color == background))
    {
        return 0;
    }
    return color;
}

void appendBoundary(std::vector<bitmapVectorPrivate::SBoundarySegment>& edges,
                    int x, int y, int direction, QRgb color, int component)
{
    if (direction == 0)
    {
        edges.push_back(
            {{x, y}, {x + 1, y}, color, component, component});
    }
    else if (direction == 1)
    {
        edges.push_back(
            {{x + 1, y}, {x + 1, y + 1}, color, component, component});
    }
    else if (direction == 2)
    {
        edges.push_back(
            {{x + 1, y + 1}, {x, y + 1}, color, component, component});
    }
    else
    {
        edges.push_back(
            {{x, y + 1}, {x, y}, color, component, component});
    }
}

} // namespace

SResult<SBitmapVectorResult> bitmapToVectorFloodFill(
    const QImage& source, const SBitmapVectorSettings& settings)
{
    if (source.isNull())
    {
        return SResult<SBitmapVectorResult>::failure(
            QStringLiteral("位图数据为空。"));
    }
    QElapsedTimer total_timer;
    total_timer.start();
    const QImage image = source.convertToFormat(QImage::Format_ARGB32);
    const QRgb background =
        settings.background_color.isValid()
        ? settings.background_color.rgba()
        : image.pixel(0, 0);
    const int width = image.width();
    const int height = image.height();
    const qint64 pixel_count = static_cast<qint64>(width) * height;
    std::vector<int> labels(static_cast<std::size_t>(pixel_count), -1);
    std::vector<int> queue;
    std::vector<bitmapVectorPrivate::SBoundarySegment> raw_segments;
    int component_count = 0;

    QElapsedTimer scan_timer;
    scan_timer.start();
    const int delta_x[] = {0, 1, 0, -1};
    const int delta_y[] = {-1, 0, 1, 0};
    for (int start_y = 0; start_y < height; ++start_y)
    {
        for (int start_x = 0; start_x < width; ++start_x)
        {
            const int start_offset = start_y * width + start_x;
            if (labels[static_cast<std::size_t>(start_offset)] >= 0)
            {
                continue;
            }
            const QRgb color =
                effectiveColor(image.pixel(start_x, start_y),
                               background, settings);
            if (qAlpha(color) == 0)
            {
                labels[static_cast<std::size_t>(start_offset)] = -2;
                continue;
            }
            queue.clear();
            queue.push_back(start_offset);
            labels[static_cast<std::size_t>(start_offset)] = component_count;
            std::size_t head = 0;
            while (head < queue.size())
            {
                const int offset = queue[head++];
                const int x = offset % width;
                const int y = offset / width;
                for (int direction = 0; direction < 4; ++direction)
                {
                    const int next_x = x + delta_x[direction];
                    const int next_y = y + delta_y[direction];
                    const bool is_inside =
                        next_x >= 0 && next_y >= 0 &&
                        next_x < width && next_y < height;
                    const QRgb next_color =
                        is_inside
                        ? effectiveColor(image.pixel(next_x, next_y),
                                         background, settings)
                        : 0;
                    if (next_color != color)
                    {
                        appendBoundary(raw_segments, x, y, direction,
                                       color, component_count);
                        continue;
                    }
                    const int next_offset = next_y * width + next_x;
                    int& label = labels[static_cast<std::size_t>(next_offset)];
                    if (label == -1)
                    {
                        label = component_count;
                        queue.push_back(next_offset);
                    }
                }
            }
            ++component_count;
        }
    }
    if (component_count == 0)
    {
        return SResult<SBitmapVectorResult>::failure(
            QStringLiteral("位图中没有可转换的前景区域。"));
    }
    const qint64 scan_nanoseconds = scan_timer.nsecsElapsed();

    QElapsedTimer connection_timer;
    connection_timer.start();
    auto compressed = bitmapVectorPrivate::compressSegments(raw_segments);
    if (!compressed)
    {
        return SResult<SBitmapVectorResult>::failure(
            compressed.errorMessage());
    }
    std::vector<bitmapVectorPrivate::SBoundarySegment> segments =
        std::move(compressed.value());
    const qint64 connection_nanoseconds =
        connection_timer.nsecsElapsed();

    QElapsedTimer stitch_timer;
    stitch_timer.start();
    auto contours = bitmapVectorPrivate::stitchSegments(segments);
    if (!contours)
    {
        return SResult<SBitmapVectorResult>::failure(
            contours.errorMessage());
    }
    SBitmapVectorResult result;
    result.contours = std::move(contours.value());
    result.svg_data =
        bitmapContoursToSvgData(width, height, result.contours);
    result.metrics.pixel_count = pixel_count;
    result.metrics.component_count = component_count;
    result.metrics.segment_count =
        static_cast<qint64>(segments.size());
    result.metrics.contour_count =
        static_cast<qint64>(result.contours.size());
    result.metrics.scan_nanoseconds = scan_nanoseconds;
    result.metrics.connection_nanoseconds = connection_nanoseconds;
    result.metrics.stitch_nanoseconds = stitch_timer.nsecsElapsed();
    result.metrics.total_nanoseconds = total_timer.nsecsElapsed();
    result.metrics.worker_count = 1;
    result.metrics.svg_bytes = result.svg_data.size();
    result.metrics.estimated_working_bytes =
        static_cast<qint64>(labels.size()) * sizeof(int) +
        static_cast<qint64>(queue.capacity()) * sizeof(int) +
        static_cast<qint64>(raw_segments.size()) *
            sizeof(bitmapVectorPrivate::SBoundarySegment) +
        static_cast<qint64>(segments.size()) *
            sizeof(bitmapVectorPrivate::SBoundarySegment);
    return SResult<SBitmapVectorResult>::success(std::move(result));
}

} // namespace smartGraphics
