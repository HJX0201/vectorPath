#include "s_cad_viewport_geometry.h"

#include <algorithm>
#include <cmath>

namespace smartGraphics
{
namespace
{

constexpr double kAngleTolerance = 1.0e-8;

double normalizedAngle(double angle)
{
    const double result = std::fmod(angle, 360.0);
    return result < 0.0 ? result + 360.0 : result;
}

double counterClockwiseSpan(double start_angle, double end_angle)
{
    return normalizedAngle(end_angle - start_angle);
}

double circularDistance(double first_angle, double second_angle)
{
    const double span = counterClockwiseSpan(first_angle, second_angle);
    return std::min(span, 360.0 - span);
}

double arcParameter(const SArcEntity& arc, const SPoint2d& point)
{
    const double total_span = counterClockwiseSpan(arc.start_angle, arc.end_angle);
    const double point_angle = entityAngleDegrees(arc.center, point);
    const double parameter = counterClockwiseSpan(arc.start_angle, point_angle);
    if (parameter <= total_span)
    {
        return parameter;
    }
    return circularDistance(point_angle, arc.start_angle) <=
                   circularDistance(point_angle, arc.end_angle)
               ? 0.0
               : total_span;
}

void appendArcPart(const SEntityRecord& source, const SArcEntity& arc, double start_parameter,
                   double end_parameter, std::vector<SEntityRecord>& results)
{
    if (end_parameter - start_parameter <= kAngleTolerance)
    {
        return;
    }
    SEntityRecord result = source;
    result.type = SEntityType::Arc;
    result.geometry =
        SArcEntity{arc.center, arc.radius, normalizedAngle(arc.start_angle + start_parameter),
                   normalizedAngle(arc.start_angle + end_parameter)};
    results.push_back(std::move(result));
}

std::vector<SEntityRecord> breakCircle(const SEntityRecord& source,
                                       const SPoint2d& first_break_point,
                                       const SPoint2d& second_break_point)
{
    const auto& circle = std::get<SCircleEntity>(source.geometry);
    const double first_angle = entityAngleDegrees(circle.center, first_break_point);
    const double second_angle = entityAngleDegrees(circle.center, second_break_point);
    if (circularDistance(first_angle, second_angle) <= kAngleTolerance)
    {
        return {};
    }
    SEntityRecord result = source;
    result.type = SEntityType::Arc;
    result.geometry = SArcEntity{circle.center, circle.radius, second_angle, first_angle};
    return {result};
}

std::vector<SEntityRecord> breakArc(const SEntityRecord& source, const SPoint2d& first_break_point,
                                    const SPoint2d& second_break_point)
{
    const auto& arc = std::get<SArcEntity>(source.geometry);
    const double total_span = counterClockwiseSpan(arc.start_angle, arc.end_angle);
    if (arc.radius <= 0.0 || total_span <= kAngleTolerance)
    {
        return {};
    }
    double first_parameter = arcParameter(arc, first_break_point);
    double second_parameter = arcParameter(arc, second_break_point);
    if (first_parameter > second_parameter)
    {
        std::swap(first_parameter, second_parameter);
    }
    if (std::abs(first_parameter - second_parameter) <= kAngleTolerance &&
        (first_parameter <= kAngleTolerance || first_parameter >= total_span - kAngleTolerance))
    {
        return {};
    }
    std::vector<SEntityRecord> results;
    appendArcPart(source, arc, 0.0, first_parameter, results);
    appendArcPart(source, arc, second_parameter, total_span, results);
    return results;
}

} // namespace

std::vector<SEntityRecord> brokenEntityParts(const SEntityRecord& source,
                                             const SPoint2d& first_break_point,
                                             const SPoint2d& second_break_point)
{
    if (source.type == SEntityType::Line)
    {
        return brokenLineEntities(source, first_break_point, second_break_point);
    }
    if (source.type == SEntityType::Circle)
    {
        return breakCircle(source, first_break_point, second_break_point);
    }
    if (source.type == SEntityType::Arc)
    {
        return breakArc(source, first_break_point, second_break_point);
    }
    return {};
}

} // namespace smartGraphics
