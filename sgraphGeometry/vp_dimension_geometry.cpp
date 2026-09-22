#include "vp_dimension_geometry.h"

#include <QtMath>
#include <algorithm>
#include <cmath>

namespace Vp
{
namespace
{

constexpr double kGeometryTolerance = 1.0e-9;
constexpr double kPi = 3.14159265358979323846;

double vectorAngle(const VpPoint2d& center, const VpPoint2d& point) noexcept
{
    return std::atan2(point.y - center.y, point.x - center.x);
}

double positiveSweep(double start_angle, double end_angle) noexcept
{
    double sweep = end_angle - start_angle;
    while (sweep < 0.0)
    {
        sweep += 2.0 * kPi;
    }
    while (sweep >= 2.0 * kPi)
    {
        sweep -= 2.0 * kPi;
    }
    return sweep > kPi ? (2.0 * kPi) - sweep : sweep;
}

double pointDistance(const VpPoint2d& first_point, const VpPoint2d& second_point) noexcept
{
    return std::hypot(second_point.x - first_point.x, second_point.y - first_point.y);
}

} // namespace

double dimensionMeasurement(const VpLinearDimensionEntity& dimension) noexcept
{
    switch (dimension.dimension_type)
    {
    case VpDimensionType::Linear:
    {
        const VpPoint2d midpoint{(dimension.first_point.x + dimension.second_point.x) * 0.5,
                                 (dimension.first_point.y + dimension.second_point.y) * 0.5};
        const double horizontal_offset = std::abs(dimension.dimension_line_point.x - midpoint.x);
        const double vertical_offset = std::abs(dimension.dimension_line_point.y - midpoint.y);
        return horizontal_offset > vertical_offset
                   ? std::abs(dimension.second_point.y - dimension.first_point.y)
                   : std::abs(dimension.second_point.x - dimension.first_point.x);
    }
    case VpDimensionType::Aligned:
        return pointDistance(dimension.first_point, dimension.second_point);
    case VpDimensionType::Angular:
        return qRadiansToDegrees(
            positiveSweep(vectorAngle(dimension.center_point, dimension.first_point),
                          vectorAngle(dimension.center_point, dimension.second_point)));
    case VpDimensionType::Radius:
        return pointDistance(dimension.center_point, dimension.first_point);
    case VpDimensionType::Diameter:
        return pointDistance(dimension.center_point, dimension.first_point) * 2.0;
    case VpDimensionType::ArcLength:
    {
        const double radius = pointDistance(dimension.center_point, dimension.first_point);
        const double sweep =
            positiveSweep(vectorAngle(dimension.center_point, dimension.first_point),
                          vectorAngle(dimension.center_point, dimension.second_point));
        return radius * sweep;
    }
    case VpDimensionType::Ordinate:
    {
        const double delta_x = std::abs(dimension.dimension_line_point.x - dimension.first_point.x);
        const double delta_y = std::abs(dimension.dimension_line_point.y - dimension.first_point.y);
        return delta_x > delta_y ? std::abs(dimension.first_point.y - dimension.center_point.y)
                                 : std::abs(dimension.first_point.x - dimension.center_point.x);
    }
    }
    return 0.0;
}

QString dimensionTypeName(VpDimensionType dimension_type)
{
    switch (dimension_type)
    {
    case VpDimensionType::Linear:
        return QStringLiteral("线性");
    case VpDimensionType::Aligned:
        return QStringLiteral("对齐");
    case VpDimensionType::Angular:
        return QStringLiteral("角度");
    case VpDimensionType::Radius:
        return QStringLiteral("半径");
    case VpDimensionType::Diameter:
        return QStringLiteral("直径");
    case VpDimensionType::ArcLength:
        return QStringLiteral("弧长");
    case VpDimensionType::Ordinate:
        return QStringLiteral("坐标");
    }
    return QStringLiteral("未知");
}

QString dimensionDefaultText(const VpLinearDimensionEntity& dimension, int linear_precision,
                             int angular_precision)
{
    if (!dimension.text_override.isEmpty())
    {
        return dimension.text_override;
    }
    const int precision = dimension.dimension_type == VpDimensionType::Angular
                              ? std::max(0, angular_precision)
                              : std::max(0, linear_precision);
    const QString value = QString::number(dimensionMeasurement(dimension), 'f', precision);
    switch (dimension.dimension_type)
    {
    case VpDimensionType::Angular:
        return value + QChar(0x00B0);
    case VpDimensionType::Radius:
        return QStringLiteral("R") + value;
    case VpDimensionType::Diameter:
        return QChar(0x2300) + value;
    case VpDimensionType::ArcLength:
        return QChar(0x2312) + value;
    default:
        return value;
    }
}

std::vector<VpPoint2d> dimensionReferencePoints(const VpLinearDimensionEntity& dimension)
{
    if (dimension.dimension_type == VpDimensionType::Angular ||
        dimension.dimension_type == VpDimensionType::ArcLength)
    {
        return {dimension.center_point, dimension.first_point, dimension.second_point,
                dimension.dimension_line_point};
    }
    if (dimension.dimension_type == VpDimensionType::Radius ||
        dimension.dimension_type == VpDimensionType::Diameter ||
        dimension.dimension_type == VpDimensionType::Ordinate)
    {
        return {dimension.center_point, dimension.first_point, dimension.dimension_line_point};
    }
    return {dimension.first_point, dimension.second_point, dimension.dimension_line_point};
}

bool isDimensionValid(const VpLinearDimensionEntity& dimension) noexcept
{
    if (!std::isfinite(dimension.first_point.x) || !std::isfinite(dimension.first_point.y) ||
        !std::isfinite(dimension.second_point.x) || !std::isfinite(dimension.second_point.y) ||
        !std::isfinite(dimension.dimension_line_point.x) ||
        !std::isfinite(dimension.dimension_line_point.y) ||
        !std::isfinite(dimension.center_point.x) || !std::isfinite(dimension.center_point.y))
    {
        return false;
    }
    if (dimension.dimension_type == VpDimensionType::Angular ||
        dimension.dimension_type == VpDimensionType::ArcLength)
    {
        return pointDistance(dimension.center_point, dimension.first_point) > kGeometryTolerance &&
               pointDistance(dimension.center_point, dimension.second_point) > kGeometryTolerance;
    }
    if (dimension.dimension_type == VpDimensionType::Radius ||
        dimension.dimension_type == VpDimensionType::Diameter)
    {
        return pointDistance(dimension.center_point, dimension.first_point) > kGeometryTolerance;
    }
    if (dimension.dimension_type == VpDimensionType::Ordinate)
    {
        return pointDistance(dimension.first_point, dimension.dimension_line_point) >
               kGeometryTolerance;
    }
    return pointDistance(dimension.first_point, dimension.second_point) > kGeometryTolerance;
}

} // namespace Vp
