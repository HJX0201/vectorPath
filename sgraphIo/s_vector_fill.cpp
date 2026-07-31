#include "s_vector_fill.h"

#include <clipper2/clipper.h>
#include <algorithm>
#include <cmath>
#include <exception>

namespace vectorPath
{
namespace
{

constexpr double kPi = 3.14159265358979323846;
constexpr double kGeometryTolerance = 1.0e-9;
constexpr int kClipperPrecision = 6;

SPoint2d rotatePoint(const SPoint2d& point, double cosine, double sine)
{
    return {cosine * point.x - sine * point.y,
            sine * point.x + cosine * point.y};
}

SVectorRegion scaledRegion(const SVectorRegion& source, double scale)
{
    SVectorRegion result = source;
    for (std::vector<SPoint2d>& contour : result.contours)
    {
        for (SPoint2d& point : contour)
        {
            point.x *= scale;
            point.y *= scale;
        }
    }
    return result;
}

Clipper2Lib::PathsD clipperPaths(const SVectorRegion& region)
{
    Clipper2Lib::PathsD result;
    result.reserve(region.contours.size());
    for (const std::vector<SPoint2d>& contour : region.contours)
    {
        Clipper2Lib::PathD path;
        path.reserve(contour.size());
        for (const SPoint2d& point : contour)
        {
            path.emplace_back(point.x, point.y);
        }
        result.push_back(std::move(path));
    }
    return result;
}

Clipper2Lib::FillRule clipperFillRule(SVectorFillRule fill_rule)
{
    return fill_rule == SVectorFillRule::EvenOdd ? Clipper2Lib::FillRule::EvenOdd
                                                  : Clipper2Lib::FillRule::NonZero;
}

void addClosedPathPolyline(const Clipper2Lib::PathD& path,
                           std::vector<SPolylineEntity>& polylines)
{
    if (path.size() < 3)
    {
        return;
    }
    std::vector<SPoint2d> points;
    points.reserve(path.size());
    for (const Clipper2Lib::PointD& point : path)
    {
        if (points.empty() ||
            std::hypot(point.x - points.back().x, point.y - points.back().y) >
                kGeometryTolerance)
        {
            points.push_back({point.x, point.y});
        }
    }
    if (points.size() > 2 &&
        std::hypot(points.front().x - points.back().x,
                   points.front().y - points.back().y) <= kGeometryTolerance)
    {
        points.pop_back();
    }
    if (points.size() >= 3)
    {
        polylines.push_back(
            {std::move(points), true, {}, {}, {}});
    }
}

std::vector<SPoint2d> pointsFromClipperPath(const Clipper2Lib::PathD& path)
{
    std::vector<SPoint2d> result;
    result.reserve(path.size());
    for (const Clipper2Lib::PointD& point : path)
    {
        result.push_back({point.x, point.y});
    }
    return result;
}

void appendHatchesFromPolyPath(const Clipper2Lib::PolyPathD& parent,
                               std::vector<SHatchEntity>& hatches)
{
    for (std::size_t index = 0; index < parent.Count(); ++index)
    {
        const Clipper2Lib::PolyPathD* child = parent.Child(index);
        if (!child)
        {
            continue;
        }
        if (!child->IsHole() && child->Polygon().size() >= 3)
        {
            SHatchEntity hatch;
            hatch.boundary = pointsFromClipperPath(child->Polygon());
            for (std::size_t nested_index = 0; nested_index < child->Count(); ++nested_index)
            {
                const Clipper2Lib::PolyPathD* nested = child->Child(nested_index);
                if (nested && nested->IsHole() && nested->Polygon().size() >= 3)
                {
                    hatch.island_boundaries.push_back(
                        pointsFromClipperPath(nested->Polygon()));
                }
            }
            hatches.push_back(std::move(hatch));
        }
        appendHatchesFromPolyPath(*child, hatches);
    }
}

struct SScanEvent
{
    double x = 0.0;
    int winding_delta = 0;
};

std::vector<SScanEvent> scanEvents(const SVectorRegion& region, double y)
{
    std::vector<SScanEvent> events;
    for (const std::vector<SPoint2d>& contour : region.contours)
    {
        for (std::size_t index = 0; index < contour.size(); ++index)
        {
            const SPoint2d& first = contour[index];
            const SPoint2d& second = contour[(index + 1) % contour.size()];
            const bool crosses_up = first.y <= y && second.y > y;
            const bool crosses_down = second.y <= y && first.y > y;
            if (!crosses_up && !crosses_down)
            {
                continue;
            }
            const double ratio = (y - first.y) / (second.y - first.y);
            events.push_back(
                {first.x + ratio * (second.x - first.x), crosses_up ? 1 : -1});
        }
    }
    std::sort(events.begin(), events.end(),
              [](const SScanEvent& first, const SScanEvent& second)
              {
                  return first.x < second.x;
              });
    return events;
}

void addScanSegment(std::vector<SLineEntity>& lines, double first_x, double second_x,
                    double y, double cosine, double sine)
{
    if (second_x - first_x <= kGeometryTolerance)
    {
        return;
    }
    lines.push_back({rotatePoint({first_x, y}, cosine, sine),
                     rotatePoint({second_x, y}, cosine, sine)});
}

std::vector<SLineEntity> evenOddScanLines(const SVectorRegion& region, double y,
                                          double cosine, double sine)
{
    const std::vector<SScanEvent> events = scanEvents(region, y);
    std::vector<SLineEntity> result;
    for (std::size_t index = 1; index < events.size(); index += 2)
    {
        addScanSegment(result, events[index - 1].x, events[index].x, y, cosine, sine);
    }
    return result;
}

std::vector<SLineEntity> nonZeroScanLines(const SVectorRegion& region, double y,
                                          double cosine, double sine)
{
    const std::vector<SScanEvent> events = scanEvents(region, y);
    std::vector<SLineEntity> result;
    int winding = 0;
    double start_x = 0.0;
    std::size_t index = 0;
    while (index < events.size())
    {
        const double x = events[index].x;
        int delta = 0;
        while (index < events.size() &&
               std::abs(events[index].x - x) <= kGeometryTolerance)
        {
            delta += events[index].winding_delta;
            ++index;
        }
        const int previous_winding = winding;
        winding += delta;
        if (previous_winding == 0 && winding != 0)
        {
            start_x = x;
        }
        else if (previous_winding != 0 && winding == 0)
        {
            addScanSegment(result, start_x, x, y, cosine, sine);
        }
    }
    return result;
}

} // namespace

SResult<std::vector<SPolylineEntity>> createSingleLineFill(
    const SVectorRegion& source_region, double spacing, double angle_degrees)
{
    if (spacing <= 0.0 || !std::isfinite(spacing) || !std::isfinite(angle_degrees))
    {
        return SResult<std::vector<SPolylineEntity>>::failure(
            QStringLiteral("单线填充间距必须大于零，角度必须有效。"));
    }
    if (source_region.contours.empty())
    {
        return SResult<std::vector<SPolylineEntity>>::success({});
    }
    const double angle = angle_degrees * kPi / 180.0;
    const double cosine = std::cos(angle);
    const double sine = std::sin(angle);
    SVectorRegion rotated = source_region;
    double minimum_y = 0.0;
    double maximum_y = 0.0;
    bool has_point = false;
    for (std::vector<SPoint2d>& contour : rotated.contours)
    {
        for (SPoint2d& point : contour)
        {
            point = rotatePoint(point, cosine, -sine);
            minimum_y = has_point ? std::min(minimum_y, point.y) : point.y;
            maximum_y = has_point ? std::max(maximum_y, point.y) : point.y;
            has_point = true;
        }
    }
    std::vector<SPolylineEntity> result;
    for (double y = minimum_y + spacing; y < maximum_y - kGeometryTolerance; y += spacing)
    {
        std::vector<SLineEntity> scan_lines =
            rotated.fill_rule == SVectorFillRule::EvenOdd
            ? evenOddScanLines(rotated, y, cosine, sine)
            : nonZeroScanLines(rotated, y, cosine, sine);
        for (const SLineEntity& line : scan_lines)
        {
            result.push_back(
                {{line.start_point, line.end_point}, false, {}, {}, {}});
        }
    }
    return SResult<std::vector<SPolylineEntity>>::success(std::move(result));
}

SResult<std::vector<SPolylineEntity>> createPolygonOffsetFill(
    const SVectorRegion& region, double spacing)
{
    if (spacing <= 0.0 || !std::isfinite(spacing))
    {
        return SResult<std::vector<SPolylineEntity>>::failure(
            QStringLiteral("多边形偏移线填充间距必须大于零。"));
    }
    try
    {
        const Clipper2Lib::PathsD normalized =
            Clipper2Lib::Union(clipperPaths(region), clipperFillRule(region.fill_rule),
                               kClipperPrecision);
        if (normalized.empty())
        {
            return SResult<std::vector<SPolylineEntity>>::success({});
        }
        double minimum_x = normalized.front().front().x;
        double maximum_x = minimum_x;
        double minimum_y = normalized.front().front().y;
        double maximum_y = minimum_y;
        for (const Clipper2Lib::PathD& path : normalized)
        {
            for (const Clipper2Lib::PointD& point : path)
            {
                minimum_x = std::min(minimum_x, point.x);
                maximum_x = std::max(maximum_x, point.x);
                minimum_y = std::min(minimum_y, point.y);
                maximum_y = std::max(maximum_y, point.y);
            }
        }
        const int maximum_layers =
            std::min(100000, std::max(1, static_cast<int>(
                std::ceil(std::max(maximum_x - minimum_x, maximum_y - minimum_y) /
                          (2.0 * spacing))) + 2));
        std::vector<SPolylineEntity> result;
        for (int layer = 1; layer <= maximum_layers; ++layer)
        {
            const Clipper2Lib::PathsD offset =
                Clipper2Lib::InflatePaths(normalized, -spacing * layer,
                                          Clipper2Lib::JoinType::Miter,
                                          Clipper2Lib::EndType::Polygon, 2.0,
                                          kClipperPrecision);
            if (offset.empty())
            {
                break;
            }
            for (const Clipper2Lib::PathD& path : offset)
            {
                addClosedPathPolyline(path, result);
            }
        }
        return SResult<std::vector<SPolylineEntity>>::success(std::move(result));
    }
    catch (const std::exception& exception)
    {
        return SResult<std::vector<SPolylineEntity>>::failure(
            QStringLiteral("多边形偏移填充失败：%1")
                .arg(QString::fromLocal8Bit(exception.what())));
    }
}

SResult<std::vector<SHatchEntity>> createRegionHatches(const SVectorRegion& region)
{
    if (region.contours.empty())
    {
        return SResult<std::vector<SHatchEntity>>::success({});
    }
    try
    {
        Clipper2Lib::PolyTreeD tree;
        Clipper2Lib::BooleanOp(Clipper2Lib::ClipType::Union,
                               clipperFillRule(region.fill_rule),
                               clipperPaths(region), {}, tree, kClipperPrecision);
        std::vector<SHatchEntity> result;
        appendHatchesFromPolyPath(tree, result);
        return SResult<std::vector<SHatchEntity>>::success(std::move(result));
    }
    catch (const std::exception& exception)
    {
        return SResult<std::vector<SHatchEntity>>::failure(
            QStringLiteral("SVG 色块边界构建失败：%1")
                .arg(QString::fromLocal8Bit(exception.what())));
    }
}

SResult<SVectorImportGeometry> createVectorImportGeometry(
    const SSvgVectorData& vector_data, const SVectorImportSettings& settings)
{
    if (settings.scale <= 0.0 || !std::isfinite(settings.scale))
    {
        return SResult<SVectorImportGeometry>::failure(
            QStringLiteral("导入比例必须大于零。"));
    }
    SVectorImportGeometry result;
    result.width = vector_data.source_width * settings.scale;
    result.height = vector_data.source_height * settings.scale;
    result.region_count = vector_data.regions.size();
    result.warnings = vector_data.warnings;
    for (const SVectorRegion& source_region : vector_data.regions)
    {
        const SVectorRegion region = scaledRegion(source_region, settings.scale);
        if (settings.include_color_blocks)
        {
            const SResult<std::vector<SHatchEntity>> hatch_result =
                createRegionHatches(region);
            if (!hatch_result)
            {
                return SResult<SVectorImportGeometry>::failure(
                    hatch_result.errorMessage());
            }
            for (const SHatchEntity& hatch : hatch_result.value())
            {
                result.entities.push_back(
                    {hatch, SEntityType::Hatch, region.color});
            }
        }
        SResult<std::vector<SPolylineEntity>> fill_result =
            SResult<std::vector<SPolylineEntity>>::success({});
        if (settings.fill_mode == SImportFillMode::SingleLine)
        {
            fill_result = createSingleLineFill(region, settings.fill_spacing,
                                               settings.fill_angle_degrees);
        }
        else if (settings.fill_mode == SImportFillMode::PolygonOffset)
        {
            fill_result = createPolygonOffsetFill(region, settings.fill_spacing);
        }
        if (!fill_result)
        {
            return SResult<SVectorImportGeometry>::failure(fill_result.errorMessage());
        }
        for (const SPolylineEntity& polyline : fill_result.value())
        {
            result.entities.push_back(
                {polyline, SEntityType::Polyline, region.color});
        }
    }
    if (settings.preserve_outlines)
    {
        for (const SVectorOutline& outline : vector_data.outlines)
        {
            std::vector<SPoint2d> points = outline.points;
            for (SPoint2d& point : points)
            {
                point.x *= settings.scale;
                point.y *= settings.scale;
            }
            if (points.size() == 2)
            {
                result.entities.push_back(
                    {SLineEntity{points[0], points[1]}, SEntityType::Line, outline.color});
            }
            else if (points.size() > 2)
            {
                result.entities.push_back(
                    {SPolylineEntity{std::move(points), outline.is_closed, {}, {}, {}},
                     SEntityType::Polyline, outline.color});
            }
        }
    }
    return SResult<SVectorImportGeometry>::success(std::move(result));
}

} // namespace vectorPath
