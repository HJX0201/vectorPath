#include "s_cad_viewport_geometry.h"

#include <algorithm>
#include <cmath>

namespace smartGraphics
{
namespace
{

bool sourcePolyline(const SEntityRecord& source, SPolylineEntity& polyline)
{
    if (source.type != SEntityType::Polyline)
    {
        return false;
    }
    polyline = std::get<SPolylineEntity>(source.geometry);
    if (polyline.vertices.size() < 2)
    {
        return false;
    }
    polyline.bulges.resize(polyline.vertices.size(), 0.0);
    polyline.start_widths.resize(polyline.vertices.size(), 0.0);
    polyline.end_widths.resize(polyline.vertices.size(), 0.0);
    return true;
}

} // namespace

bool polylineClosedStateEntity(const SEntityRecord& source, bool is_closed, SEntityRecord& result)
{
    SPolylineEntity polyline;
    if (!sourcePolyline(source, polyline) || (is_closed && polyline.vertices.size() < 3))
    {
        return false;
    }
    polyline.is_closed = is_closed;
    if (!is_closed)
    {
        polyline.bulges.back() = 0.0;
        polyline.start_widths.back() = 0.0;
        polyline.end_widths.back() = 0.0;
    }
    result = source;
    result.geometry = std::move(polyline);
    return true;
}

bool polylineConstantWidthEntity(const SEntityRecord& source, double width, SEntityRecord& result)
{
    SPolylineEntity polyline;
    if (!sourcePolyline(source, polyline) || !std::isfinite(width) || width < 0.0 || width > 1.0e9)
    {
        return false;
    }
    const std::size_t segment_count =
        polyline.is_closed ? polyline.vertices.size() : polyline.vertices.size() - 1;
    std::fill(polyline.start_widths.begin(), polyline.start_widths.end(), 0.0);
    std::fill(polyline.end_widths.begin(), polyline.end_widths.end(), 0.0);
    std::fill_n(polyline.start_widths.begin(), segment_count, width);
    std::fill_n(polyline.end_widths.begin(), segment_count, width);
    result = source;
    result.geometry = std::move(polyline);
    return true;
}

bool reversedPolylineEntity(const SEntityRecord& source, SEntityRecord& result)
{
    SPolylineEntity polyline;
    if (!sourcePolyline(source, polyline))
    {
        return false;
    }
    SPolylineEntity reversed;
    reversed.vertices.assign(polyline.vertices.rbegin(), polyline.vertices.rend());
    reversed.is_closed = polyline.is_closed;
    reversed.bulges.resize(polyline.vertices.size(), 0.0);
    reversed.start_widths.resize(polyline.vertices.size(), 0.0);
    reversed.end_widths.resize(polyline.vertices.size(), 0.0);
    const std::size_t segment_count =
        polyline.is_closed ? polyline.vertices.size() : polyline.vertices.size() - 1;
    for (std::size_t index = 0; index < segment_count; ++index)
    {
        const std::size_t source_index =
            (polyline.vertices.size() + polyline.vertices.size() - 2 - index) %
            polyline.vertices.size();
        reversed.bulges[index] = -polyline.bulges[source_index];
        reversed.start_widths[index] = polyline.end_widths[source_index];
        reversed.end_widths[index] = polyline.start_widths[source_index];
    }
    result = source;
    result.geometry = std::move(reversed);
    return true;
}

bool decurvedPolylineEntity(const SEntityRecord& source, SEntityRecord& result)
{
    SPolylineEntity polyline;
    if (!sourcePolyline(source, polyline))
    {
        return false;
    }
    std::fill(polyline.bulges.begin(), polyline.bulges.end(), 0.0);
    result = source;
    result.geometry = std::move(polyline);
    return true;
}

} // namespace smartGraphics
