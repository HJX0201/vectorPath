#include "s_cad_viewport_geometry.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace smartCam
{
namespace
{

constexpr double kFullCircleDegrees = 360.0;

double normalizedAngle(double angle)
{
    const double result = std::fmod(angle, kFullCircleDegrees);
    return result < 0.0 ? result + kFullCircleDegrees : result;
}

double counterClockwiseSpan(double start_angle, double end_angle)
{
    return normalizedAngle(end_angle - start_angle);
}

SPoint2d arcPoint(const SArcEntity& arc, double angle)
{
    constexpr double kPi = 3.14159265358979323846;
    const double radians = angle * kPi / 180.0;
    return {arc.center.x + (arc.radius * std::cos(radians)),
            arc.center.y + (arc.radius * std::sin(radians))};
}

bool joinedArcEntity(const std::vector<SEntityRecord>& sources, double tolerance,
                     SEntityRecord& result)
{
    if (sources.size() < 2 || tolerance <= 0.0)
    {
        return false;
    }
    const auto& reference = std::get<SArcEntity>(sources.front().geometry);
    for (const SEntityRecord& source : sources)
    {
        if (source.type != SEntityType::Arc)
        {
            return false;
        }
        const auto& arc = std::get<SArcEntity>(source.geometry);
        if (distance(reference.center, arc.center) > tolerance ||
            std::abs(reference.radius - arc.radius) > tolerance || arc.radius <= tolerance)
        {
            return false;
        }
    }

    std::vector<std::size_t> order;
    order.reserve(sources.size());
    std::size_t first_index = sources.size();
    for (std::size_t candidate = 0; candidate < sources.size(); ++candidate)
    {
        const auto& candidate_arc = std::get<SArcEntity>(sources[candidate].geometry);
        const SPoint2d candidate_start = arcPoint(candidate_arc, candidate_arc.start_angle);
        bool has_predecessor = false;
        for (std::size_t other = 0; other < sources.size(); ++other)
        {
            const auto& other_arc = std::get<SArcEntity>(sources[other].geometry);
            if (other != candidate &&
                distance(candidate_start, arcPoint(other_arc, other_arc.end_angle)) <= tolerance)
            {
                has_predecessor = true;
                break;
            }
        }
        if (!has_predecessor)
        {
            if (first_index != sources.size())
            {
                return false;
            }
            first_index = candidate;
        }
    }
    const bool is_closed_chain = first_index == sources.size();
    if (is_closed_chain)
    {
        first_index = 0;
    }

    std::vector<bool> used(sources.size(), false);
    std::size_t current_index = first_index;
    while (order.size() < sources.size())
    {
        order.push_back(current_index);
        used[current_index] = true;
        const auto& current_arc = std::get<SArcEntity>(sources[current_index].geometry);
        const SPoint2d current_end = arcPoint(current_arc, current_arc.end_angle);
        std::size_t next_index = sources.size();
        for (std::size_t candidate = 0; candidate < sources.size(); ++candidate)
        {
            if (used[candidate])
            {
                continue;
            }
            const auto& candidate_arc = std::get<SArcEntity>(sources[candidate].geometry);
            if (distance(current_end, arcPoint(candidate_arc, candidate_arc.start_angle)) <=
                tolerance)
            {
                if (next_index != sources.size())
                {
                    return false;
                }
                next_index = candidate;
            }
        }
        if (next_index == sources.size())
        {
            break;
        }
        current_index = next_index;
    }
    if (order.size() != sources.size())
    {
        return false;
    }

    double total_span = 0.0;
    for (std::size_t index : order)
    {
        const auto& arc = std::get<SArcEntity>(sources[index].geometry);
        total_span += counterClockwiseSpan(arc.start_angle, arc.end_angle);
    }
    const double angular_tolerance =
        (tolerance / reference.radius) * 180.0 / 3.14159265358979323846;
    if (total_span > kFullCircleDegrees + angular_tolerance)
    {
        return false;
    }
    result = sources[order.front()];
    if (is_closed_chain && std::abs(total_span - kFullCircleDegrees) <= angular_tolerance)
    {
        result.type = SEntityType::Circle;
        result.geometry = SCircleEntity{reference.center, reference.radius};
        return true;
    }
    const auto& first_arc = std::get<SArcEntity>(sources[order.front()].geometry);
    const auto& last_arc = std::get<SArcEntity>(sources[order.back()].geometry);
    result.geometry =
        SArcEntity{reference.center, reference.radius, first_arc.start_angle, last_arc.end_angle};
    return !is_closed_chain;
}

struct SPolylinePath
{
    std::vector<SPoint2d> vertices;
    std::vector<double> bulges;
    std::vector<double> start_widths;
    std::vector<double> end_widths;
};

SPolylinePath reversedPath(const SPolylinePath& source)
{
    SPolylinePath result;
    result.vertices.assign(source.vertices.rbegin(), source.vertices.rend());
    result.bulges.resize(source.vertices.size(), 0.0);
    result.start_widths.resize(source.vertices.size(), 0.0);
    result.end_widths.resize(source.vertices.size(), 0.0);
    for (std::size_t index = 0; index + 1 < source.vertices.size(); ++index)
    {
        const std::size_t source_index = source.vertices.size() - 2 - index;
        result.bulges[index] = -source.bulges[source_index];
        result.start_widths[index] = source.end_widths[source_index];
        result.end_widths[index] = source.start_widths[source_index];
    }
    return result;
}

void appendPath(SPolylinePath& destination, const SPolylinePath& addition)
{
    destination.bulges.back() = addition.bulges.front();
    destination.start_widths.back() = addition.start_widths.front();
    destination.end_widths.back() = addition.end_widths.front();
    destination.vertices.insert(destination.vertices.end(), addition.vertices.begin() + 1,
                                addition.vertices.end());
    destination.bulges.insert(destination.bulges.end(), addition.bulges.begin() + 1,
                              addition.bulges.end());
    destination.start_widths.insert(destination.start_widths.end(),
                                    addition.start_widths.begin() + 1, addition.start_widths.end());
    destination.end_widths.insert(destination.end_widths.end(), addition.end_widths.begin() + 1,
                                  addition.end_widths.end());
}

bool joinedLinearEntity(const std::vector<SEntityRecord>& sources, double tolerance,
                        SEntityRecord& result)
{
    if (sources.size() < 2 || tolerance <= 0.0)
    {
        return false;
    }
    std::vector<SPolylinePath> paths;
    paths.reserve(sources.size());
    for (const SEntityRecord& source : sources)
    {
        if (source.type == SEntityType::Line)
        {
            const auto& line = std::get<SLineEntity>(source.geometry);
            paths.push_back(
                {{line.start_point, line.end_point}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}});
            continue;
        }
        if (source.type != SEntityType::Polyline)
        {
            return false;
        }
        const auto& polyline = std::get<SPolylineEntity>(source.geometry);
        if (polyline.is_closed || polyline.vertices.size() < 2)
        {
            return false;
        }
        SPolylinePath path{polyline.vertices, polyline.bulges, polyline.start_widths,
                           polyline.end_widths};
        path.bulges.resize(path.vertices.size(), 0.0);
        path.start_widths.resize(path.vertices.size(), 0.0);
        path.end_widths.resize(path.vertices.size(), 0.0);
        paths.push_back(std::move(path));
    }
    SPolylinePath joined = paths.front();
    std::vector<bool> used(paths.size(), false);
    used.front() = true;
    std::size_t used_count = 1;
    while (used_count < paths.size())
    {
        bool did_connect = false;
        for (std::size_t index = 1; index < paths.size(); ++index)
        {
            if (used[index])
            {
                continue;
            }
            SPolylinePath candidate = paths[index];
            if (distance(joined.vertices.back(), candidate.vertices.front()) <= tolerance)
            {
                appendPath(joined, candidate);
            }
            else if (distance(joined.vertices.back(), candidate.vertices.back()) <= tolerance)
            {
                candidate = reversedPath(candidate);
                appendPath(joined, candidate);
            }
            else if (distance(candidate.vertices.back(), joined.vertices.front()) <= tolerance)
            {
                appendPath(candidate, joined);
                joined = std::move(candidate);
            }
            else if (distance(candidate.vertices.front(), joined.vertices.front()) <= tolerance)
            {
                candidate = reversedPath(candidate);
                appendPath(candidate, joined);
                joined = std::move(candidate);
            }
            else
            {
                continue;
            }
            used[index] = true;
            ++used_count;
            did_connect = true;
            break;
        }
        if (!did_connect)
        {
            return false;
        }
    }
    const bool is_closed = joined.vertices.size() > 3 &&
                           distance(joined.vertices.front(), joined.vertices.back()) <= tolerance;
    if (is_closed)
    {
        joined.vertices.pop_back();
        joined.bulges.pop_back();
        joined.start_widths.pop_back();
        joined.end_widths.pop_back();
    }
    result = sources.front();
    result.type = SEntityType::Polyline;
    result.geometry =
        SPolylineEntity{std::move(joined.vertices), is_closed, std::move(joined.bulges),
                        std::move(joined.start_widths), std::move(joined.end_widths)};
    return true;
}

} // namespace

bool joinedEntity(const std::vector<SEntityRecord>& sources, double tolerance,
                  SEntityRecord& result)
{
    if (sources.empty())
    {
        return false;
    }
    if (sources.front().type == SEntityType::Line || sources.front().type == SEntityType::Polyline)
    {
        return joinedLinearEntity(sources, tolerance, result);
    }
    if (sources.front().type == SEntityType::Arc)
    {
        return joinedArcEntity(sources, tolerance, result);
    }
    return false;
}

} // namespace smartCam
