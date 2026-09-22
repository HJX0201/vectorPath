#include "vp_bitmap_vector_private.h"
#include "vp_bitmap_vectorizer.h"
#include "vp_qt_text.h"

#include <QElapsedTimer>
#include <QFuture>
#include <QSet>
#include <QThread>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>
#include <functional>
#include <numeric>
#include <utility>

namespace Vp
{
namespace
{

using bitmapVectorPrivate::VpBoundarySegment;

constexpr qint64 SParallelPixelThreshold = 1024 * 1024;

struct VpColorRun
{
    int row = 0;
    int begin_x = 0;
    int end_x = 0;
    QRgb color = 0;
    int run_id = -1;
    bool is_foreground = false;
};

struct VpRowConnection
{
    std::vector<std::pair<int, int>> joins;
    std::vector<VpBoundarySegment> segments;
};

class VpDisjointSet
{
  public:
    explicit VpDisjointSet(int size)
        : m_parent(static_cast<std::size_t>(size)), m_rank(static_cast<std::size_t>(size), 0)
    {
        std::iota(m_parent.begin(), m_parent.end(), 0);
    }

    int find(int value)
    {
        int root = value;
        while (m_parent[static_cast<std::size_t>(root)] != root)
        {
            root = m_parent[static_cast<std::size_t>(root)];
        }
        while (m_parent[static_cast<std::size_t>(value)] != value)
        {
            const int parent = m_parent[static_cast<std::size_t>(value)];
            m_parent[static_cast<std::size_t>(value)] = root;
            value = parent;
        }
        return root;
    }

    void unite(int first, int second)
    {
        int first_root = find(first);
        int second_root = find(second);
        if (first_root == second_root)
        {
            return;
        }
        quint8& first_rank = m_rank[static_cast<std::size_t>(first_root)];
        quint8& second_rank = m_rank[static_cast<std::size_t>(second_root)];
        if (first_rank < second_rank)
        {
            std::swap(first_root, second_root);
        }
        m_parent[static_cast<std::size_t>(second_root)] = first_root;
        if (first_rank == second_rank)
        {
            ++first_rank;
        }
    }

  private:
    std::vector<int> m_parent;
    std::vector<quint8> m_rank;
};

QRgb effectiveColor(QRgb color, QRgb background, const VpBitmapVectorSettings& settings)
{
    if (qAlpha(color) == 0 || (settings.ignore_background && color == background))
    {
        return 0;
    }
    return color;
}

std::vector<VpColorRun> extractRowRuns(const QImage& image, int row, QRgb background,
                                       const VpBitmapVectorSettings& settings)
{
    std::vector<VpColorRun> runs;
    const QRgb* pixels = reinterpret_cast<const QRgb*>(image.constScanLine(row));
    int begin_x = 0;
    QRgb color = effectiveColor(pixels[0], background, settings);
    for (int x = 1; x <= image.width(); ++x)
    {
        const QRgb next_color =
            x < image.width() ? effectiveColor(pixels[x], background, settings) : ~color;
        if (x == image.width() || next_color != color)
        {
            runs.push_back({row, begin_x, x, color, -1, qAlpha(color) != 0});
            begin_x = x;
            color = next_color;
        }
    }
    return runs;
}

int requestedWorkerCount(const QImage& image, const VpBitmapVectorSettings& settings)
{
    if (settings.worker_count > 0)
    {
        return std::max(1, settings.worker_count);
    }
    const qint64 pixels = static_cast<qint64>(image.width()) * image.height();
    if (pixels < SParallelPixelThreshold)
    {
        return 1;
    }
    return std::max(1, QThread::idealThreadCount());
}

void parallelRanges(int item_count, int worker_count,
                    const std::function<void(int, int)>& operation)
{
    if (item_count <= 0)
    {
        return;
    }
    if (worker_count <= 1)
    {
        operation(0, item_count);
        return;
    }
    const int task_count = std::min(item_count, worker_count * 4);
    const int chunk_size = (item_count + task_count - 1) / task_count;
    QThreadPool pool;
    pool.setMaxThreadCount(worker_count);
    std::vector<QFuture<void>> futures;
    for (int begin = 0; begin < item_count; begin += chunk_size)
    {
        const int end = std::min(item_count, begin + chunk_size);
        futures.push_back(QtConcurrent::run(&pool,
                                            [begin, end, &operation]()
                                            {
                                                operation(begin, end);
                                            }));
    }
    for (QFuture<void>& future : futures)
    {
        future.waitForFinished();
    }
}

void connectRows(const std::vector<VpColorRun>& upper, const std::vector<VpColorRun>& lower,
                 int boundary_y, VpRowConnection& result)
{
    std::size_t upper_index = 0;
    std::size_t lower_index = 0;
    while (upper_index < upper.size() && lower_index < lower.size())
    {
        const VpColorRun& first = upper[upper_index];
        const VpColorRun& second = lower[lower_index];
        const int begin_x = std::max(first.begin_x, second.begin_x);
        const int end_x = std::min(first.end_x, second.end_x);
        if (begin_x < end_x)
        {
            if (first.is_foreground && second.is_foreground && first.color == second.color)
            {
                result.joins.emplace_back(first.run_id, second.run_id);
            }
            else
            {
                if (first.is_foreground)
                {
                    result.segments.push_back({{end_x, boundary_y},
                                               {begin_x, boundary_y},
                                               first.color,
                                               first.run_id,
                                               -1});
                }
                if (second.is_foreground)
                {
                    result.segments.push_back({{begin_x, boundary_y},
                                               {end_x, boundary_y},
                                               second.color,
                                               second.run_id,
                                               -1});
                }
            }
        }
        if (first.end_x == end_x)
        {
            ++upper_index;
        }
        if (second.end_x == end_x)
        {
            ++lower_index;
        }
    }
}

} // namespace

VpResult<VpBitmapVectorResult> bitmapToVectorResult(const QImage& source,
                                                    const VpBitmapVectorSettings& settings)
{
    if (source.isNull() || source.width() <= 0 || source.height() <= 0)
    {
        return VpResult<VpBitmapVectorResult>::failure(
            toCoreText(QStringLiteral("位图数据为空。")));
    }
    QElapsedTimer total_timer;
    total_timer.start();
    const QImage image = source.convertToFormat(QImage::Format_ARGB32);
    const QRgb background =
        settings.background_color.isValid() ? settings.background_color.rgba() : image.pixel(0, 0);
    const int worker_count = requestedWorkerCount(image, settings);

    QElapsedTimer phase_timer;
    phase_timer.start();
    std::vector<std::vector<VpColorRun>> rows(static_cast<std::size_t>(image.height()));
    parallelRanges(image.height(), worker_count,
                   [&](int begin, int end)
                   {
                       for (int row = begin; row < end; ++row)
                       {
                           rows[static_cast<std::size_t>(row)] =
                               extractRowRuns(image, row, background, settings);
                       }
                   });
    int next_run_id = 0;
    for (std::vector<VpColorRun>& row : rows)
    {
        for (VpColorRun& run : row)
        {
            if (run.is_foreground)
            {
                run.run_id = next_run_id++;
            }
        }
    }
    if (next_run_id == 0)
    {
        return VpResult<VpBitmapVectorResult>::failure(
            toCoreText(QStringLiteral("位图中没有可转换的前景区域。")));
    }
    const qint64 scan_nanoseconds = phase_timer.nsecsElapsed();

    phase_timer.restart();
    std::vector<VpRowConnection> connections(static_cast<std::size_t>(image.height() + 1));
    for (const VpColorRun& run : rows.front())
    {
        if (run.is_foreground)
        {
            connections.front().segments.push_back(
                {{run.begin_x, 0}, {run.end_x, 0}, run.color, run.run_id, -1});
        }
    }
    for (const VpColorRun& run : rows.back())
    {
        if (run.is_foreground)
        {
            connections.back().segments.push_back({{run.end_x, image.height()},
                                                   {run.begin_x, image.height()},
                                                   run.color,
                                                   run.run_id,
                                                   -1});
        }
    }
    parallelRanges(std::max(0, image.height() - 1), worker_count,
                   [&](int begin, int end)
                   {
                       for (int index = begin; index < end; ++index)
                       {
                           connectRows(rows[static_cast<std::size_t>(index)],
                                       rows[static_cast<std::size_t>(index + 1)], index + 1,
                                       connections[static_cast<std::size_t>(index + 1)]);
                       }
                   });

    VpDisjointSet disjoint_set(next_run_id);
    for (const VpRowConnection& connection : connections)
    {
        for (const std::pair<int, int>& join : connection.joins)
        {
            disjoint_set.unite(join.first, join.second);
        }
    }
    QSet<int> component_roots;
    std::vector<VpBoundarySegment> raw_segments;
    for (const VpRowConnection& connection : connections)
    {
        raw_segments.insert(raw_segments.end(), connection.segments.begin(),
                            connection.segments.end());
    }
    for (const std::vector<VpColorRun>& row : rows)
    {
        for (const VpColorRun& run : row)
        {
            if (!run.is_foreground)
            {
                continue;
            }
            const int block_id = disjoint_set.find(run.run_id);
            component_roots.insert(block_id);
            raw_segments.push_back({{run.begin_x, run.row + 1},
                                    {run.begin_x, run.row},
                                    run.color,
                                    run.run_id,
                                    block_id});
            raw_segments.push_back(
                {{run.end_x, run.row}, {run.end_x, run.row + 1}, run.color, run.run_id, block_id});
        }
    }
    for (VpBoundarySegment& segment : raw_segments)
    {
        if (segment.block_id < 0)
        {
            segment.block_id = disjoint_set.find(segment.owner_id);
        }
    }
    VpResult<std::vector<VpBoundarySegment>> compressed_result =
        bitmapVectorPrivate::compressSegments(raw_segments);
    if (!compressed_result)
    {
        return VpResult<VpBitmapVectorResult>::failure(toCoreText(toQtError(compressed_result)));
    }
    std::vector<VpBoundarySegment> segments = std::move(compressed_result.value());
    const qint64 connection_nanoseconds = phase_timer.nsecsElapsed();

    phase_timer.restart();
    VpResult<std::vector<VpBitmapContour>> contour_result =
        bitmapVectorPrivate::stitchSegments(segments);
    if (!contour_result)
    {
        return VpResult<VpBitmapVectorResult>::failure(toCoreText(toQtError(contour_result)));
    }
    VpBitmapVectorResult result;
    result.contours = std::move(contour_result.value());
    result.svg_data = bitmapContoursToSvgData(image.width(), image.height(), result.contours);
    result.metrics.pixel_count = static_cast<qint64>(image.width()) * image.height();
    result.metrics.run_count = next_run_id;
    result.metrics.component_count = component_roots.size();
    result.metrics.segment_count = static_cast<qint64>(segments.size());
    result.metrics.contour_count = static_cast<qint64>(result.contours.size());
    result.metrics.scan_nanoseconds = scan_nanoseconds;
    result.metrics.connection_nanoseconds = connection_nanoseconds;
    result.metrics.stitch_nanoseconds = phase_timer.nsecsElapsed();
    result.metrics.total_nanoseconds = total_timer.nsecsElapsed();
    result.metrics.worker_count = worker_count;
    result.metrics.svg_bytes = result.svg_data.size();
    result.metrics.estimated_working_bytes =
        static_cast<qint64>(next_run_id) * sizeof(VpColorRun) +
        static_cast<qint64>(raw_segments.size()) * sizeof(VpBoundarySegment) +
        static_cast<qint64>(segments.size()) * sizeof(VpBoundarySegment);
    return VpResult<VpBitmapVectorResult>::success(std::move(result));
}

} // namespace Vp
