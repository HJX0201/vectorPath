#include "s_cad_viewport_geometry.h"
#include "s_circle_tangent_math.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace vectorPath
{
namespace
{

using detail::appendQuadraticRoots;
using detail::constraintIntersection;
using detail::SLinearConstraint;
using detail::SVector3;

constexpr double kPi = 3.14159265358979323846;
constexpr double kEpsilon = 1.0e-9;

struct SCurveSupport
{
    bool is_line = false;
    bool is_arc = false;
    SPoint2d first;
    SPoint2d second;
    SPoint2d center;
    double radius = 0.0;
    double start_angle = 0.0;
    double end_angle = 0.0;
};

struct SLineEquation
{
    double normal_x = 0.0;
    double normal_y = 0.0;
    double constant = 0.0;
};

bool supportFromEntity(const SEntityRecord& entity, SCurveSupport& support)
{
    if (entity.type == SEntityType::Line)
    {
        const auto& line = std::get<SLineEntity>(entity.geometry);
        if (distance(line.start_point, line.end_point) <= kEpsilon)
        {
            return false;
        }
        support.is_line = true;
        support.first = line.start_point;
        support.second = line.end_point;
        return true;
    }
    if (entity.type == SEntityType::Circle)
    {
        const auto& circle = std::get<SCircleEntity>(entity.geometry);
        support.center = circle.center;
        support.radius = circle.radius;
        return circle.radius > kEpsilon;
    }
    if (entity.type == SEntityType::Arc)
    {
        const auto& arc = std::get<SArcEntity>(entity.geometry);
        support.is_arc = true;
        support.center = arc.center;
        support.radius = arc.radius;
        support.start_angle = arc.start_angle;
        support.end_angle = arc.end_angle;
        return arc.radius > kEpsilon;
    }
    return false;
}

SLineEquation lineEquation(const SCurveSupport& support)
{
    const double delta_x = support.second.x - support.first.x;
    const double delta_y = support.second.y - support.first.y;
    const double length = std::hypot(delta_x, delta_y);
    const double normal_x = -delta_y / length;
    const double normal_y = delta_x / length;
    return {normal_x, normal_y, (normal_x * support.first.x) + (normal_y * support.first.y)};
}

double normalizedAngle(double angle)
{
    double result = std::fmod(angle, 360.0);
    if (result < 0.0)
    {
        result += 360.0;
    }
    return result;
}

bool pointOnArc(const SCurveSupport& support, const SPoint2d& point)
{
    if (!support.is_arc)
    {
        return true;
    }
    const double angle = normalizedAngle(
        std::atan2(point.y - support.center.y, point.x - support.center.x) * 180.0 / kPi);
    const double span = normalizedAngle(support.end_angle - support.start_angle);
    const double offset = normalizedAngle(angle - support.start_angle);
    return offset <= span + 1.0e-6;
}

void appendLineLineIntersections(const SLineEquation& first, const SLineEquation& second,
                                 double first_offset, double second_offset,
                                 std::vector<SPoint2d>& centers)
{
    const double determinant =
        (first.normal_x * second.normal_y) - (first.normal_y * second.normal_x);
    if (std::abs(determinant) <= kEpsilon)
    {
        return;
    }
    const double first_constant = first.constant + first_offset;
    const double second_constant = second.constant + second_offset;
    centers.push_back(
        {((first_constant * second.normal_y) - (first.normal_y * second_constant)) / determinant,
         ((first.normal_x * second_constant) - (first_constant * second.normal_x)) / determinant});
}

void appendLineCircleIntersections(const SLineEquation& line, double line_offset,
                                   const SPoint2d& center, double radius,
                                   std::vector<SPoint2d>& centers)
{
    const double constant = line.constant + line_offset;
    const double signed_distance =
        (line.normal_x * center.x) + (line.normal_y * center.y) - constant;
    if (std::abs(signed_distance) > radius + kEpsilon)
    {
        return;
    }
    const SPoint2d projection{center.x - (line.normal_x * signed_distance),
                              center.y - (line.normal_y * signed_distance)};
    const double tangent_offset =
        std::sqrt(std::max(0.0, (radius * radius) - (signed_distance * signed_distance)));
    const SPoint2d direction{-line.normal_y, line.normal_x};
    centers.push_back({projection.x + (direction.x * tangent_offset),
                       projection.y + (direction.y * tangent_offset)});
    if (tangent_offset > kEpsilon)
    {
        centers.push_back({projection.x - (direction.x * tangent_offset),
                           projection.y - (direction.y * tangent_offset)});
    }
}

void appendCircleCircleIntersections(const SPoint2d& first_center, double first_radius,
                                     const SPoint2d& second_center, double second_radius,
                                     std::vector<SPoint2d>& centers)
{
    const double center_distance = distance(first_center, second_center);
    if (center_distance <= kEpsilon || center_distance > first_radius + second_radius + kEpsilon ||
        center_distance < std::abs(first_radius - second_radius) - kEpsilon)
    {
        return;
    }
    const double along = ((first_radius * first_radius) - (second_radius * second_radius) +
                          (center_distance * center_distance)) /
                         (2.0 * center_distance);
    const double height = std::sqrt(std::max(0.0, (first_radius * first_radius) - (along * along)));
    const double unit_x = (second_center.x - first_center.x) / center_distance;
    const double unit_y = (second_center.y - first_center.y) / center_distance;
    const SPoint2d base{first_center.x + (unit_x * along), first_center.y + (unit_y * along)};
    centers.push_back({base.x - (unit_y * height), base.y + (unit_x * height)});
    if (height > kEpsilon)
    {
        centers.push_back({base.x + (unit_y * height), base.y - (unit_x * height)});
    }
}

std::vector<double> circleLocusRadii(double source_radius, double tangent_radius)
{
    std::vector<double> radii{source_radius + tangent_radius};
    const double difference = std::abs(source_radius - tangent_radius);
    if (difference > kEpsilon && std::abs(difference - radii.front()) > kEpsilon)
    {
        radii.push_back(difference);
    }
    return radii;
}

bool tangencyPoint(const SCurveSupport& support, const SPoint2d& candidate_center,
                   double candidate_radius, SPoint2d& point)
{
    if (support.is_line)
    {
        const SLineEquation line = lineEquation(support);
        const double signed_distance = (line.normal_x * candidate_center.x) +
                                       (line.normal_y * candidate_center.y) - line.constant;
        if (std::abs(std::abs(signed_distance) - candidate_radius) > 1.0e-5)
        {
            return false;
        }
        point = {candidate_center.x - (line.normal_x * signed_distance),
                 candidate_center.y - (line.normal_y * signed_distance)};
        return true;
    }
    const double center_distance = distance(support.center, candidate_center);
    if (center_distance <= kEpsilon)
    {
        return false;
    }
    const double unit_x = (candidate_center.x - support.center.x) / center_distance;
    const double unit_y = (candidate_center.y - support.center.y) / center_distance;
    const double tolerance = 1.0e-5 * std::max({1.0, support.radius, candidate_radius});
    if (std::abs(std::abs(center_distance - support.radius) - candidate_radius) <= tolerance)
    {
        point = {support.center.x + (unit_x * support.radius),
                 support.center.y + (unit_y * support.radius)};
    }
    else if (std::abs((center_distance + support.radius) - candidate_radius) <= tolerance)
    {
        point = {support.center.x - (unit_x * support.radius),
                 support.center.y - (unit_y * support.radius)};
    }
    else
    {
        return false;
    }
    return pointOnArc(support, point);
}

double determinant3(const std::array<std::array<double, 3>, 3>& matrix)
{
    return matrix[0][0] * ((matrix[1][1] * matrix[2][2]) - (matrix[1][2] * matrix[2][1])) -
           matrix[0][1] * ((matrix[1][0] * matrix[2][2]) - (matrix[1][2] * matrix[2][0])) +
           matrix[0][2] * ((matrix[1][0] * matrix[2][1]) - (matrix[1][1] * matrix[2][0]));
}

struct STangentBranch
{
    double line_sign = 0.0;
    double radial_scale = 0.0;
    double radial_offset = 0.0;
};

struct SCurveEquation
{
    SVector3 linear;
    double constant = 0.0;
};

std::array<STangentBranch, 3> contactBranches(const SCurveSupport& support,
                                              std::size_t& branch_count)
{
    if (support.is_line)
    {
        branch_count = 2;
        return {STangentBranch{-1.0, 0.0, 0.0}, STangentBranch{1.0, 0.0, 0.0}, STangentBranch{}};
    }
    branch_count = 3;
    return {STangentBranch{0.0, 1.0, support.radius}, STangentBranch{0.0, -1.0, support.radius},
            STangentBranch{0.0, 1.0, -support.radius}};
}

SLinearConstraint lineConstraint(const SCurveSupport& support, const STangentBranch& branch)
{
    const SLineEquation line = lineEquation(support);
    return {{line.normal_x, line.normal_y, -branch.line_sign}, line.constant};
}

SCurveEquation curveEquation(const SCurveSupport& support, const STangentBranch& branch)
{
    return {{-2.0 * support.center.x, -2.0 * support.center.y,
             -2.0 * branch.radial_scale * branch.radial_offset},
            (support.center.x * support.center.x) + (support.center.y * support.center.y) -
                (branch.radial_offset * branch.radial_offset)};
}

SLinearConstraint curveDifferenceConstraint(const SCurveSupport& first_support,
                                            const STangentBranch& first_branch,
                                            const SCurveSupport& second_support,
                                            const STangentBranch& second_branch)
{
    const SCurveEquation first = curveEquation(first_support, first_branch);
    const SCurveEquation second = curveEquation(second_support, second_branch);
    return {{first.linear.x - second.linear.x, first.linear.y - second.linear.y,
             first.linear.z - second.linear.z},
            second.constant - first.constant};
}

bool branchSatisfied(const SCurveSupport& support, const STangentBranch& branch,
                     const SCircleEntity& candidate)
{
    const double tolerance = 1.0e-6 * std::max({1.0, support.radius, candidate.radius,
                                                distance(support.center, candidate.center)});
    if (support.is_line)
    {
        const SLineEquation line = lineEquation(support);
        const double signed_distance = (line.normal_x * candidate.center.x) +
                                       (line.normal_y * candidate.center.y) - line.constant;
        return std::abs(signed_distance - (branch.line_sign * candidate.radius)) <= tolerance;
    }
    const double required_distance =
        (branch.radial_scale * candidate.radius) + branch.radial_offset;
    return required_distance >= -tolerance &&
           std::abs(distance(support.center, candidate.center) - required_distance) <= tolerance;
}

bool sameCircle(const SCircleEntity& first, const SCircleEntity& second)
{
    const double scale = std::max({1.0, first.radius, second.radius});
    return distance(first.center, second.center) <= 1.0e-7 * scale &&
           std::abs(first.radius - second.radius) <= 1.0e-7 * scale;
}

void appendThreeEntityCandidates(const std::array<SCurveSupport, 3>& supports,
                                 const std::array<STangentBranch, 3>& branches,
                                 std::vector<SCircleEntity>& candidates)
{
    std::array<std::size_t, 3> line_indices{};
    std::array<std::size_t, 3> curve_indices{};
    std::size_t line_count = 0;
    std::size_t curve_count = 0;
    for (std::size_t index = 0; index < supports.size(); ++index)
    {
        if (supports[index].is_line)
        {
            line_indices[line_count++] = index;
        }
        else
        {
            curve_indices[curve_count++] = index;
        }
    }
    if (curve_count == 0)
    {
        return;
    }

    SLinearConstraint first_constraint;
    SLinearConstraint second_constraint;
    if (line_count == 2)
    {
        first_constraint = lineConstraint(supports[line_indices[0]], branches[line_indices[0]]);
        second_constraint = lineConstraint(supports[line_indices[1]], branches[line_indices[1]]);
    }
    else if (line_count == 1)
    {
        first_constraint = lineConstraint(supports[line_indices[0]], branches[line_indices[0]]);
        second_constraint =
            curveDifferenceConstraint(supports[curve_indices[1]], branches[curve_indices[1]],
                                      supports[curve_indices[0]], branches[curve_indices[0]]);
    }
    else
    {
        first_constraint =
            curveDifferenceConstraint(supports[curve_indices[1]], branches[curve_indices[1]],
                                      supports[curve_indices[0]], branches[curve_indices[0]]);
        second_constraint =
            curveDifferenceConstraint(supports[curve_indices[2]], branches[curve_indices[2]],
                                      supports[curve_indices[0]], branches[curve_indices[0]]);
    }

    SVector3 base;
    SVector3 direction;
    if (!constraintIntersection(first_constraint, second_constraint, base, direction))
    {
        return;
    }
    const std::size_t target_index = curve_indices[0];
    const SCurveSupport& target = supports[target_index];
    const STangentBranch& target_branch = branches[target_index];
    const double delta_x = base.x - target.center.x;
    const double delta_y = base.y - target.center.y;
    const double radial_base = (target_branch.radial_scale * base.z) + target_branch.radial_offset;
    const double radial_direction = target_branch.radial_scale * direction.z;
    std::vector<double> roots;
    appendQuadraticRoots((direction.x * direction.x) + (direction.y * direction.y) -
                             (radial_direction * radial_direction),
                         2.0 * ((delta_x * direction.x) + (delta_y * direction.y) -
                                (radial_base * radial_direction)),
                         (delta_x * delta_x) + (delta_y * delta_y) - (radial_base * radial_base),
                         roots);
    for (double root : roots)
    {
        const SCircleEntity candidate{
            {base.x + (direction.x * root), base.y + (direction.y * root)},
            base.z + (direction.z * root)};
        if (!std::isfinite(candidate.center.x) || !std::isfinite(candidate.center.y) ||
            !std::isfinite(candidate.radius) || candidate.radius <= kEpsilon)
        {
            continue;
        }
        bool is_valid = true;
        for (std::size_t index = 0; index < supports.size(); ++index)
        {
            SPoint2d tangent;
            if (!branchSatisfied(supports[index], branches[index], candidate) ||
                !tangencyPoint(supports[index], candidate.center, candidate.radius, tangent))
            {
                is_valid = false;
                break;
            }
        }
        if (is_valid && std::none_of(candidates.begin(), candidates.end(),
                                     [&candidate](const auto& existing)
                                     {
                                         return sameCircle(existing, candidate);
                                     }))
        {
            candidates.push_back(candidate);
        }
    }
}

} // namespace

bool tangentCircleToTwoEntities(const SEntityRecord& first_source,
                                const SEntityRecord& second_source, const SPoint2d& first_pick,
                                const SPoint2d& second_pick, double radius, SCircleEntity& result)
{
    if (!std::isfinite(radius) || radius <= kEpsilon)
    {
        return false;
    }
    SCurveSupport first;
    SCurveSupport second;
    if (!supportFromEntity(first_source, first) || !supportFromEntity(second_source, second))
    {
        return false;
    }
    std::vector<SPoint2d> centers;
    if (first.is_line && second.is_line)
    {
        const SLineEquation first_line = lineEquation(first);
        const SLineEquation second_line = lineEquation(second);
        for (double first_sign : {-1.0, 1.0})
        {
            for (double second_sign : {-1.0, 1.0})
            {
                appendLineLineIntersections(first_line, second_line, first_sign * radius,
                                            second_sign * radius, centers);
            }
        }
    }
    else if (first.is_line || second.is_line)
    {
        const SCurveSupport& line_support = first.is_line ? first : second;
        const SCurveSupport& circle_support = first.is_line ? second : first;
        const SLineEquation line = lineEquation(line_support);
        for (double line_sign : {-1.0, 1.0})
        {
            for (double locus_radius : circleLocusRadii(circle_support.radius, radius))
            {
                appendLineCircleIntersections(line, line_sign * radius, circle_support.center,
                                              locus_radius, centers);
            }
        }
    }
    else
    {
        for (double first_radius : circleLocusRadii(first.radius, radius))
        {
            for (double second_radius : circleLocusRadii(second.radius, radius))
            {
                appendCircleCircleIntersections(first.center, first_radius, second.center,
                                                second_radius, centers);
            }
        }
    }

    double best_score = std::numeric_limits<double>::max();
    bool has_result = false;
    for (const SPoint2d& center : centers)
    {
        SPoint2d first_tangent;
        SPoint2d second_tangent;
        if (!tangencyPoint(first, center, radius, first_tangent) ||
            !tangencyPoint(second, center, radius, second_tangent))
        {
            continue;
        }
        const double score =
            distance(first_tangent, first_pick) + distance(second_tangent, second_pick);
        if (score < best_score)
        {
            best_score = score;
            result = {center, radius};
            has_result = true;
        }
    }
    return has_result;
}

bool tangentCircleToThreeLines(const std::array<SEntityRecord, 3>& sources,
                               const std::array<SPoint2d, 3>& picks, SCircleEntity& result)
{
    std::array<SCurveSupport, 3> supports;
    std::array<SLineEquation, 3> lines;
    for (std::size_t index = 0; index < sources.size(); ++index)
    {
        if (!supportFromEntity(sources[index], supports[index]) || !supports[index].is_line)
        {
            return false;
        }
        lines[index] = lineEquation(supports[index]);
    }
    double best_score = std::numeric_limits<double>::max();
    bool has_result = false;
    for (int first_sign : {-1, 1})
    {
        for (int second_sign : {-1, 1})
        {
            for (int third_sign : {-1, 1})
            {
                const std::array<int, 3> signs{first_sign, second_sign, third_sign};
                std::array<std::array<double, 3>, 3> matrix{};
                std::array<double, 3> constants{};
                for (std::size_t index = 0; index < 3; ++index)
                {
                    matrix[index] = {lines[index].normal_x, lines[index].normal_y,
                                     -static_cast<double>(signs[index])};
                    constants[index] = lines[index].constant;
                }
                const double determinant = determinant3(matrix);
                if (std::abs(determinant) <= kEpsilon)
                {
                    continue;
                }
                auto x_matrix = matrix;
                auto y_matrix = matrix;
                auto radius_matrix = matrix;
                for (std::size_t row = 0; row < 3; ++row)
                {
                    x_matrix[row][0] = constants[row];
                    y_matrix[row][1] = constants[row];
                    radius_matrix[row][2] = constants[row];
                }
                const SPoint2d center{determinant3(x_matrix) / determinant,
                                      determinant3(y_matrix) / determinant};
                const double radius = determinant3(radius_matrix) / determinant;
                if (!std::isfinite(radius) || radius <= kEpsilon)
                {
                    continue;
                }
                double score = 0.0;
                for (std::size_t index = 0; index < 3; ++index)
                {
                    SPoint2d tangent;
                    if (!tangencyPoint(supports[index], center, radius, tangent))
                    {
                        score = std::numeric_limits<double>::max();
                        break;
                    }
                    score += distance(tangent, picks[index]);
                }
                if (score < best_score)
                {
                    best_score = score;
                    result = {center, radius};
                    has_result = true;
                }
            }
        }
    }
    return has_result;
}

bool tangentCircleToThreeEntities(const std::array<SEntityRecord, 3>& sources,
                                  const std::array<SPoint2d, 3>& picks, SCircleEntity& result)
{
    std::array<SCurveSupport, 3> supports;
    bool all_lines = true;
    for (std::size_t index = 0; index < sources.size(); ++index)
    {
        if (!supportFromEntity(sources[index], supports[index]))
        {
            return false;
        }
        all_lines = all_lines && supports[index].is_line;
    }
    if (all_lines)
    {
        return tangentCircleToThreeLines(sources, picks, result);
    }

    std::array<std::array<STangentBranch, 3>, 3> branch_sets;
    std::array<std::size_t, 3> branch_counts{};
    for (std::size_t index = 0; index < supports.size(); ++index)
    {
        branch_sets[index] = contactBranches(supports[index], branch_counts[index]);
    }
    std::vector<SCircleEntity> candidates;
    for (std::size_t first_index = 0; first_index < branch_counts[0]; ++first_index)
    {
        for (std::size_t second_index = 0; second_index < branch_counts[1]; ++second_index)
        {
            for (std::size_t third_index = 0; third_index < branch_counts[2]; ++third_index)
            {
                appendThreeEntityCandidates(supports,
                                            {branch_sets[0][first_index],
                                             branch_sets[1][second_index],
                                             branch_sets[2][third_index]},
                                            candidates);
            }
        }
    }

    double best_score = std::numeric_limits<double>::max();
    bool has_result = false;
    for (const SCircleEntity& candidate : candidates)
    {
        double score = 0.0;
        for (std::size_t index = 0; index < supports.size(); ++index)
        {
            SPoint2d tangent;
            if (!tangencyPoint(supports[index], candidate.center, candidate.radius, tangent))
            {
                score = std::numeric_limits<double>::max();
                break;
            }
            score += distance(tangent, picks[index]);
        }
        if (score < best_score)
        {
            best_score = score;
            result = candidate;
            has_result = true;
        }
    }
    return has_result;
}

} // namespace vectorPath
