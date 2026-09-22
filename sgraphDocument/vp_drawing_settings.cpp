#include "vp_drawing_settings.h"

#include <cmath>

namespace Vp
{

bool operator==(const VpDrawingSettings& left, const VpDrawingSettings& right) noexcept
{
    return left.insertion_unit == right.insertion_unit && left.angle_format == right.angle_format &&
           left.linear_precision == right.linear_precision &&
           left.angular_precision == right.angular_precision;
}

bool operator!=(const VpDrawingSettings& left, const VpDrawingSettings& right) noexcept
{
    return !(left == right);
}

bool isValidDrawingSettings(const VpDrawingSettings& settings) noexcept
{
    const bool is_unit_valid = settings.insertion_unit >= VpInsertionUnit::Unitless &&
                               settings.insertion_unit <= VpInsertionUnit::Feet;
    const bool is_angle_format_valid = settings.angle_format >= VpAngleFormat::DecimalDegrees &&
                                       settings.angle_format <= VpAngleFormat::Radians;
    return is_unit_valid && is_angle_format_valid && settings.linear_precision >= 0 &&
           settings.linear_precision <= 8 && settings.angular_precision >= 0 &&
           settings.angular_precision <= 8;
}

QString insertionUnitKey(VpInsertionUnit unit)
{
    switch (unit)
    {
    case VpInsertionUnit::Unitless:
        return QStringLiteral("unitless");
    case VpInsertionUnit::Millimeters:
        return QStringLiteral("millimeter");
    case VpInsertionUnit::Centimeters:
        return QStringLiteral("centimeter");
    case VpInsertionUnit::Meters:
        return QStringLiteral("meter");
    case VpInsertionUnit::Inches:
        return QStringLiteral("inch");
    case VpInsertionUnit::Feet:
        return QStringLiteral("foot");
    }
    return QStringLiteral("unitless");
}

QString insertionUnitSymbol(VpInsertionUnit unit)
{
    switch (unit)
    {
    case VpInsertionUnit::Millimeters:
        return QStringLiteral("mm");
    case VpInsertionUnit::Centimeters:
        return QStringLiteral("cm");
    case VpInsertionUnit::Meters:
        return QStringLiteral("m");
    case VpInsertionUnit::Inches:
        return QStringLiteral("in");
    case VpInsertionUnit::Feet:
        return QStringLiteral("ft");
    case VpInsertionUnit::Unitless:
        return {};
    }
    return {};
}

std::optional<VpInsertionUnit> insertionUnitFromKey(const QString& key)
{
    const QString normalized_key = key.trimmed().toLower();
    if (normalized_key == QLatin1String("unitless") || normalized_key == QLatin1String("none"))
    {
        return VpInsertionUnit::Unitless;
    }
    if (normalized_key == QLatin1String("millimeter") || normalized_key == QLatin1String("mm"))
    {
        return VpInsertionUnit::Millimeters;
    }
    if (normalized_key == QLatin1String("centimeter") || normalized_key == QLatin1String("cm"))
    {
        return VpInsertionUnit::Centimeters;
    }
    if (normalized_key == QLatin1String("meter") || normalized_key == QLatin1String("m"))
    {
        return VpInsertionUnit::Meters;
    }
    if (normalized_key == QLatin1String("inch") || normalized_key == QLatin1String("inches"))
    {
        return VpInsertionUnit::Inches;
    }
    if (normalized_key == QLatin1String("foot") || normalized_key == QLatin1String("feet"))
    {
        return VpInsertionUnit::Feet;
    }
    return std::nullopt;
}

QString angleFormatKey(VpAngleFormat format)
{
    switch (format)
    {
    case VpAngleFormat::DecimalDegrees:
        return QStringLiteral("decimal_degrees");
    case VpAngleFormat::DegreesMinutesSeconds:
        return QStringLiteral("degrees_minutes_seconds");
    case VpAngleFormat::Gradians:
        return QStringLiteral("gradians");
    case VpAngleFormat::Radians:
        return QStringLiteral("radians");
    }
    return QStringLiteral("decimal_degrees");
}

std::optional<VpAngleFormat> angleFormatFromKey(const QString& key)
{
    const QString normalized_key = key.trimmed().toLower();
    if (normalized_key == QLatin1String("decimal_degrees") ||
        normalized_key == QLatin1String("decimal") || normalized_key == QLatin1String("degree"))
    {
        return VpAngleFormat::DecimalDegrees;
    }
    if (normalized_key == QLatin1String("degrees_minutes_seconds") ||
        normalized_key == QLatin1String("dms"))
    {
        return VpAngleFormat::DegreesMinutesSeconds;
    }
    if (normalized_key == QLatin1String("gradians") || normalized_key == QLatin1String("grads"))
    {
        return VpAngleFormat::Gradians;
    }
    if (normalized_key == QLatin1String("radians") || normalized_key == QLatin1String("radian"))
    {
        return VpAngleFormat::Radians;
    }
    return std::nullopt;
}

QString formatLinearValue(double value, const VpDrawingSettings& settings)
{
    const QString number = QString::number(value, 'f', settings.linear_precision);
    const QString symbol = insertionUnitSymbol(settings.insertion_unit);
    return symbol.isEmpty() ? number : QStringLiteral("%1 %2").arg(number, symbol);
}

QString formatAngleValue(double angle_degrees, const VpDrawingSettings& settings)
{
    constexpr double kPi = 3.14159265358979323846;
    if (settings.angle_format == VpAngleFormat::Radians)
    {
        return QStringLiteral("%1 rad").arg(
            QString::number(angle_degrees * kPi / 180.0, 'f', settings.angular_precision));
    }
    if (settings.angle_format == VpAngleFormat::Gradians)
    {
        return QStringLiteral("%1g").arg(
            QString::number(angle_degrees * 10.0 / 9.0, 'f', settings.angular_precision));
    }
    if (settings.angle_format == VpAngleFormat::DegreesMinutesSeconds)
    {
        const double absolute_angle = std::abs(angle_degrees);
        const int degrees = static_cast<int>(std::floor(absolute_angle));
        const double minutes_value = (absolute_angle - degrees) * 60.0;
        const int minutes = static_cast<int>(std::floor(minutes_value));
        const double seconds = (minutes_value - minutes) * 60.0;
        return QStringLiteral("%1%2°%3′%4″")
            .arg(angle_degrees < 0.0 ? QStringLiteral("-") : QString())
            .arg(degrees)
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 0, 'f', settings.angular_precision);
    }
    return QStringLiteral("%1°").arg(angle_degrees, 0, 'f', settings.angular_precision);
}

} // namespace Vp
