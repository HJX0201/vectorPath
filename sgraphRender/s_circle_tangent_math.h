#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace vectorPath
{
namespace detail
{

struct SVector3
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct SLinearConstraint
{
    SVector3 normal;
    double value = 0.0;
};

inline double dotProduct(const SVector3& first, const SVector3& second)
{
    return (first.x * second.x) + (first.y * second.y) + (first.z * second.z);
}

inline SVector3 crossProduct(const SVector3& first, const SVector3& second)
{
    return {(first.y * second.z) - (first.z * second.y),
            (first.z * second.x) - (first.x * second.z),
            (first.x * second.y) - (first.y * second.x)};
}

inline bool constraintIntersection(const SLinearConstraint& first, const SLinearConstraint& second,
                                   SVector3& base, SVector3& direction)
{
    const double first_length = dotProduct(first.normal, first.normal);
    const double second_length = dotProduct(second.normal, second.normal);
    const double shared = dotProduct(first.normal, second.normal);
    const double determinant = (first_length * second_length) - (shared * shared);
    const double scale = std::max(1.0, first_length * second_length);
    if (determinant <= 1.0e-12 * scale)
    {
        return false;
    }
    const double first_weight =
        ((first.value * second_length) - (second.value * shared)) / determinant;
    const double second_weight =
        ((second.value * first_length) - (first.value * shared)) / determinant;
    base = {(first_weight * first.normal.x) + (second_weight * second.normal.x),
            (first_weight * first.normal.y) + (second_weight * second.normal.y),
            (first_weight * first.normal.z) + (second_weight * second.normal.z)};
    direction = crossProduct(first.normal, second.normal);
    const double direction_length = std::sqrt(dotProduct(direction, direction));
    if (direction_length <= 1.0e-12)
    {
        return false;
    }
    direction.x /= direction_length;
    direction.y /= direction_length;
    direction.z /= direction_length;
    return true;
}

inline void appendQuadraticRoots(double coefficient_a, double coefficient_b, double coefficient_c,
                                 std::vector<double>& roots)
{
    const double coefficient_scale =
        std::max({1.0, std::abs(coefficient_a), std::abs(coefficient_b), std::abs(coefficient_c)});
    if (std::abs(coefficient_a) <= 1.0e-12 * coefficient_scale)
    {
        if (std::abs(coefficient_b) > 1.0e-12 * coefficient_scale)
        {
            roots.push_back(-coefficient_c / coefficient_b);
        }
        return;
    }
    double discriminant = (coefficient_b * coefficient_b) - (4.0 * coefficient_a * coefficient_c);
    const double discriminant_scale = std::max(
        {1.0, coefficient_b * coefficient_b, std::abs(4.0 * coefficient_a * coefficient_c)});
    if (discriminant < -1.0e-10 * discriminant_scale)
    {
        return;
    }
    discriminant = std::max(0.0, discriminant);
    const double square_root = std::sqrt(discriminant);
    const double stable_numerator =
        -0.5 * (coefficient_b + std::copysign(square_root, coefficient_b));
    if (std::abs(stable_numerator) <= 1.0e-12 * coefficient_scale)
    {
        roots.push_back(-coefficient_b / (2.0 * coefficient_a));
        return;
    }
    roots.push_back(stable_numerator / coefficient_a);
    const double second_root = coefficient_c / stable_numerator;
    if (std::abs(second_root - roots.back()) > 1.0e-8)
    {
        roots.push_back(second_root);
    }
}

} // namespace detail
} // namespace vectorPath
