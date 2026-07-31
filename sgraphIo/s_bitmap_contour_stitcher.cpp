#include "s_bitmap_vector_private.h"

#include <QHash>
#include <QVector>
#include <algorithm>
#include <tuple>

namespace smartCam
{
namespace bitmapVectorPrivate
{
namespace
{

int segmentDirection(const SBoundarySegment& segment)
{
    const QPoint delta = segment.end - segment.start;
    if (delta.x() > 0)
    {
        return 0;
    }
    if (delta.y() > 0)
    {
        return 1;
    }
    if (delta.x() < 0)
    {
        return 2;
    }
    return 3;
}

int turnPriority(int previous_direction, int next_direction)
{
    const int turn = (next_direction - previous_direction + 4) % 4;
    if (turn == 1)
    {
        return 0;
    }
    if (turn == 0)
    {
        return 1;
    }
    if (turn == 3)
    {
        return 2;
    }
    return 3;
}

quint64 pointKey(const QPoint& point)
{
    return (static_cast<quint64>(
                static_cast<quint32>(point.x())) << 32U) |
           static_cast<quint32>(point.y());
}

struct SNormalizedSegment
{
    int block_id = -1;
    QRgb color = 0;
    int direction = 0;
    int fixed = 0;
    int low = 0;
    int high = 0;
};

SNormalizedSegment normalizeSegment(const SBoundarySegment& segment)
{
    const int direction = segmentDirection(segment);
    const bool is_horizontal = direction == 0 || direction == 2;
    const int first = is_horizontal ? segment.start.x() : segment.start.y();
    const int second = is_horizontal ? segment.end.x() : segment.end.y();
    return {segment.block_id, segment.color, direction,
            is_horizontal ? segment.start.y() : segment.start.x(),
            std::min(first, second), std::max(first, second)};
}

SBoundarySegment restoreSegment(const SNormalizedSegment& segment)
{
    if (segment.direction == 0)
    {
        return {{segment.low, segment.fixed},
                {segment.high, segment.fixed},
                segment.color, -1, segment.block_id};
    }
    if (segment.direction == 1)
    {
        return {{segment.fixed, segment.low},
                {segment.fixed, segment.high},
                segment.color, -1, segment.block_id};
    }
    if (segment.direction == 2)
    {
        return {{segment.high, segment.fixed},
                {segment.low, segment.fixed},
                segment.color, -1, segment.block_id};
    }
    return {{segment.fixed, segment.high},
            {segment.fixed, segment.low},
            segment.color, -1, segment.block_id};
}

std::vector<QPoint> simplifyLoop(const std::vector<QPoint>& source)
{
    if (source.size() < 4)
    {
        return source;
    }
    std::vector<QPoint> result;
    result.reserve(source.size());
    for (std::size_t index = 0; index < source.size(); ++index)
    {
        const QPoint& previous =
            source[(index + source.size() - 1) % source.size()];
        const QPoint& current = source[index];
        const QPoint& next = source[(index + 1) % source.size()];
        const QPoint first_delta = current - previous;
        const QPoint second_delta = next - current;
        if (first_delta.x() * second_delta.y() -
                first_delta.y() * second_delta.x() !=
            0)
        {
            result.push_back(current);
        }
    }
    return result;
}

void canonicalizeLoop(std::vector<QPoint>& points)
{
    if (points.empty())
    {
        return;
    }
    auto best = points.begin();
    for (auto iterator = points.begin() + 1; iterator != points.end(); ++iterator)
    {
        if (std::make_tuple(iterator->y(), iterator->x()) <
            std::make_tuple(best->y(), best->x()))
        {
            best = iterator;
        }
    }
    std::rotate(points.begin(), best, points.end());
}

} // namespace

SResult<std::vector<SBoundarySegment>> compressSegments(
    const std::vector<SBoundarySegment>& source)
{
    std::vector<SNormalizedSegment> normalized;
    normalized.reserve(source.size());
    for (const SBoundarySegment& segment : source)
    {
        if (segment.start == segment.end)
        {
            return SResult<std::vector<SBoundarySegment>>::failure(
                QStringLiteral("位图边界包含零长度线段。"));
        }
        normalized.push_back(normalizeSegment(segment));
    }
    std::sort(normalized.begin(), normalized.end(),
              [](const SNormalizedSegment& first,
                 const SNormalizedSegment& second)
              {
                  return std::tie(first.block_id, first.color, first.direction,
                                  first.fixed, first.low, first.high) <
                         std::tie(second.block_id, second.color, second.direction,
                                  second.fixed, second.low, second.high);
              });

    std::vector<SNormalizedSegment> merged;
    for (const SNormalizedSegment& segment : normalized)
    {
        if (!merged.empty())
        {
            SNormalizedSegment& previous = merged.back();
            const bool is_same_line =
                previous.block_id == segment.block_id &&
                previous.color == segment.color &&
                previous.direction == segment.direction &&
                previous.fixed == segment.fixed;
            if (is_same_line && segment.low < previous.high)
            {
                return SResult<std::vector<SBoundarySegment>>::failure(
                    QStringLiteral("位图边界包含重复或重叠线段。"));
            }
            if (is_same_line && segment.low == previous.high)
            {
                previous.high = segment.high;
                continue;
            }
        }
        merged.push_back(segment);
    }

    std::vector<SBoundarySegment> result;
    result.reserve(merged.size());
    for (const SNormalizedSegment& segment : merged)
    {
        result.push_back(restoreSegment(segment));
    }
    return SResult<std::vector<SBoundarySegment>>::success(std::move(result));
}

SResult<std::vector<SBitmapContour>> stitchSegments(
    std::vector<SBoundarySegment>& segments)
{
    std::vector<bool> is_used(segments.size(), false);
    std::vector<SBitmapContour> contours;
    std::size_t range_begin = 0;
    while (range_begin < segments.size())
    {
        std::size_t range_end = range_begin + 1;
        while (range_end < segments.size() &&
               segments[range_end].block_id ==
                   segments[range_begin].block_id)
        {
            ++range_end;
        }
        QHash<quint64, QVector<int>> outgoing;
        for (std::size_t index = range_begin; index < range_end; ++index)
        {
            outgoing[pointKey(segments[index].start)]
                .append(static_cast<int>(index));
        }
        for (std::size_t start_index = range_begin;
             start_index < range_end; ++start_index)
        {
            if (is_used[start_index])
            {
                continue;
            }
            std::vector<QPoint> loop;
            const QPoint loop_start = segments[start_index].start;
            int edge_index = static_cast<int>(start_index);
            int previous_direction = segmentDirection(segments[start_index]);
            std::size_t guard = 0;
            while (edge_index >= 0 && guard <= range_end - range_begin)
            {
                const std::size_t current_index =
                    static_cast<std::size_t>(edge_index);
                if (is_used[current_index])
                {
                    return SResult<std::vector<SBitmapContour>>::failure(
                        QStringLiteral("位图轮廓在闭合前重复使用了线段。"));
                }
                is_used[current_index] = true;
                const SBoundarySegment& edge = segments[current_index];
                loop.push_back(edge.start);
                const QPoint vertex = edge.end;
                if (vertex == loop_start)
                {
                    break;
                }
                int best_index = -1;
                int best_priority = 5;
                bool has_tie = false;
                for (int candidate_index : outgoing.value(pointKey(vertex)))
                {
                    if (is_used[static_cast<std::size_t>(candidate_index)])
                    {
                        continue;
                    }
                    const int priority = turnPriority(
                        previous_direction,
                        segmentDirection(
                            segments[static_cast<std::size_t>(candidate_index)]));
                    if (priority < best_priority)
                    {
                        best_priority = priority;
                        best_index = candidate_index;
                        has_tie = false;
                    }
                    else if (priority == best_priority)
                    {
                        has_tie = true;
                    }
                }
                if (best_index < 0 || has_tie)
                {
                    return SResult<std::vector<SBitmapContour>>::failure(
                        QStringLiteral("位图轮廓存在开放端点或无法消解的分支。"));
                }
                edge_index = best_index;
                previous_direction =
                    segmentDirection(segments[static_cast<std::size_t>(edge_index)]);
                ++guard;
            }
            if (loop.empty() ||
                segments[static_cast<std::size_t>(edge_index)].end != loop_start)
            {
                return SResult<std::vector<SBitmapContour>>::failure(
                    QStringLiteral("位图轮廓未能闭合。"));
            }
            loop = simplifyLoop(loop);
            if (loop.size() < 3)
            {
                return SResult<std::vector<SBitmapContour>>::failure(
                    QStringLiteral("位图轮廓退化为少于三个顶点。"));
            }
            canonicalizeLoop(loop);
            contours.push_back(
                {segments[start_index].color,
                 segments[start_index].block_id, std::move(loop)});
        }
        range_begin = range_end;
    }
    std::sort(contours.begin(), contours.end(),
              [](const SBitmapContour& first, const SBitmapContour& second)
              {
                  if (first.color != second.color)
                  {
                      return first.color < second.color;
                  }
                  if (first.block_id != second.block_id)
                  {
                      return first.block_id < second.block_id;
                  }
                  return std::make_tuple(first.points.front().y(),
                                         first.points.front().x()) <
                         std::make_tuple(second.points.front().y(),
                                         second.points.front().x());
              });
    return SResult<std::vector<SBitmapContour>>::success(std::move(contours));
}

} // namespace bitmapVectorPrivate
} // namespace smartCam
