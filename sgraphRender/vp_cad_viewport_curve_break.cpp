#include "vp_cad_viewport_geometry.h"

#include <algorithm>
#include <cmath>

namespace Vp
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

double arcParameter(const VpArcEntity& arc, const VpPoint2d& point)
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

void appendArcPart(const VpEntityRecord& source, const VpArcEntity& arc, double start_parameter,
                   double end_parameter, std::vector<VpEntityRecord>& results)
{
    if (end_parameter - start_parameter <= kAngleTolerance)
    {
        return;
    }
    VpEntityRecord result = source;
    result.type = VpEntityType::Arc;
    result.geometry =
        VpArcEntity{arc.center, arc.radius, normalizedAngle(arc.start_angle + start_parameter),
                    normalizedAngle(arc.start_angle + end_parameter)};
    results.push_back(std::move(result));
}

std::vector<VpEntityRecord> breakCircle(const VpEntityRecord& source,
                                        const VpPoint2d& first_break_point,
                                        const VpPoint2d& second_break_point)
{
    const auto& circle = std::get<VpCircleEntity>(source.geometry);
    const double first_angle = entityAngleDegrees(circle.center, first_break_point);
    const double second_angle = entityAngleDegrees(circle.center, second_break_point);
    if (circularDistance(first_angle, second_angle) <= kAngleTolerance)
    {
        return {};
    }
    VpEntityRecord result = source;
    result.type = VpEntityType::Arc;
    result.geometry = VpArcEntity{circle.center, circle.radius, second_angle, first_angle};
    return {result};
}

std::vector<VpEntityRecord> breakArc(const VpEntityRecord& source,
                                     const VpPoint2d& first_break_point,
                                     const VpPoint2d& second_break_point)
{
    const auto& arc = std::get<VpArcEntity>(source.geometry);
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
    std::vector<VpEntityRecord> results;
    appendArcPart(source, arc, 0.0, first_parameter, results);
    appendArcPart(source, arc, second_parameter, total_span, results);
    return results;
}

} // namespace

std::vector<VpEntityRecord> brokenEntityParts(const VpEntityRecord& source,
                                              const VpPoint2d& first_break_point,
                                              const VpPoint2d& second_break_point)
{
    if (source.type == VpEntityType::Line)
    {
        return brokenLineEntities(source, first_break_point, second_break_point);
    }
    if (source.type == VpEntityType::Circle)
    {
        return breakCircle(source, first_break_point, second_break_point);
    }
    if (source.type == VpEntityType::Arc)
    {
        return breakArc(source, first_break_point, second_break_point);
    }
    return {};
}

} // namespace Vp
