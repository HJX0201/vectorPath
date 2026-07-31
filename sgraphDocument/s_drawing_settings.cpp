#include "s_drawing_settings.h"

#include <cmath>

namespace smartCam
{

bool operator==(const SDrawingSettings& left, const SDrawingSettings& right) noexcept
{
    return left.insertion_unit == right.insertion_unit && left.angle_format == right.angle_format &&
           left.linear_precision == right.linear_precision &&
           left.angular_precision == right.angular_precision;
}

bool operator!=(const SDrawingSettings& left, const SDrawingSettings& right) noexcept
{
    return !(left == right);
}

bool isValidDrawingSettings(const SDrawingSettings& settings) noexcept
{
    const bool is_unit_valid = settings.insertion_unit >= SInsertionUnit::Unitless &&
                               settings.insertion_unit <= SInsertionUnit::Feet;
    const bool is_angle_format_valid = settings.angle_format >= SAngleFormat::DecimalDegrees &&
                                       settings.angle_format <= SAngleFormat::Radians;
    return is_unit_valid && is_angle_format_valid && settings.linear_precision >= 0 &&
           settings.linear_precision <= 8 && settings.angular_precision >= 0 &&
           settings.angular_precision <= 8;
}

QString insertionUnitKey(SInsertionUnit unit)
{
    switch (unit)
    {
    case SInsertionUnit::Unitless:
        return QStringLiteral("unitless");
    case SInsertionUnit::Millimeters:
        return QStringLiteral("millimeter");
    case SInsertionUnit::Centimeters:
        return QStringLiteral("centimeter");
    case SInsertionUnit::Meters:
        return QStringLiteral("meter");
    case SInsertionUnit::Inches:
        return QStringLiteral("inch");
    case SInsertionUnit::Feet:
        return QStringLiteral("foot");
    }
    return QStringLiteral("unitless");
}

QString insertionUnitSymbol(SInsertionUnit unit)
{
    switch (unit)
    {
    case SInsertionUnit::Millimeters:
        return QStringLiteral("mm");
    case SInsertionUnit::Centimeters:
        return QStringLiteral("cm");
    case SInsertionUnit::Meters:
        return QStringLiteral("m");
    case SInsertionUnit::Inches:
        return QStringLiteral("in");
    case SInsertionUnit::Feet:
        return QStringLiteral("ft");
    case SInsertionUnit::Unitless:
        return {};
    }
    return {};
}

std::optional<SInsertionUnit> insertionUnitFromKey(const QString& key)
{
    const QString normalized_key = key.trimmed().toLower();
    if (normalized_key == QLatin1String("unitless") || normalized_key == QLatin1String("none"))
    {
        return SInsertionUnit::Unitless;
    }
    if (normalized_key == QLatin1String("millimeter") || normalized_key == QLatin1String("mm"))
    {
        return SInsertionUnit::Millimeters;
    }
    if (normalized_key == QLatin1String("centimeter") || normalized_key == QLatin1String("cm"))
    {
        return SInsertionUnit::Centimeters;
    }
    if (normalized_key == QLatin1String("meter") || normalized_key == QLatin1String("m"))
    {
        return SInsertionUnit::Meters;
    }
    if (normalized_key == QLatin1String("inch") || normalized_key == QLatin1String("inches"))
    {
        return SInsertionUnit::Inches;
    }
    if (normalized_key == QLatin1String("foot") || normalized_key == QLatin1String("feet"))
    {
        return SInsertionUnit::Feet;
    }
    return std::nullopt;
}

QString angleFormatKey(SAngleFormat format)
{
    switch (format)
    {
    case SAngleFormat::DecimalDegrees:
        return QStringLiteral("decimal_degrees");
    case SAngleFormat::DegreesMinutesSeconds:
        return QStringLiteral("degrees_minutes_seconds");
    case SAngleFormat::Gradians:
        return QStringLiteral("gradians");
    case SAngleFormat::Radians:
        return QStringLiteral("radians");
    }
    return QStringLiteral("decimal_degrees");
}

std::optional<SAngleFormat> angleFormatFromKey(const QString& key)
{
    const QString normalized_key = key.trimmed().toLower();
    if (normalized_key == QLatin1String("decimal_degrees") ||
        normalized_key == QLatin1String("decimal") || normalized_key == QLatin1String("degree"))
    {
        return SAngleFormat::DecimalDegrees;
    }
    if (normalized_key == QLatin1String("degrees_minutes_seconds") ||
        normalized_key == QLatin1String("dms"))
    {
        return SAngleFormat::DegreesMinutesSeconds;
    }
    if (normalized_key == QLatin1String("gradians") || normalized_key == QLatin1String("grads"))
    {
        return SAngleFormat::Gradians;
    }
    if (normalized_key == QLatin1String("radians") || normalized_key == QLatin1String("radian"))
    {
        return SAngleFormat::Radians;
    }
    return std::nullopt;
}

QString formatLinearValue(double value, const SDrawingSettings& settings)
{
    const QString number = QString::number(value, 'f', settings.linear_precision);
    const QString symbol = insertionUnitSymbol(settings.insertion_unit);
    return symbol.isEmpty() ? number : QStringLiteral("%1 %2").arg(number, symbol);
}

QString formatAngleValue(double angle_degrees, const SDrawingSettings& settings)
{
    constexpr double kPi = 3.14159265358979323846;
    if (settings.angle_format == SAngleFormat::Radians)
    {
        return QStringLiteral("%1 rad").arg(
            QString::number(angle_degrees * kPi / 180.0, 'f', settings.angular_precision));
    }
    if (settings.angle_format == SAngleFormat::Gradians)
    {
        return QStringLiteral("%1g").arg(
            QString::number(angle_degrees * 10.0 / 9.0, 'f', settings.angular_precision));
    }
    if (settings.angle_format == SAngleFormat::DegreesMinutesSeconds)
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

} // namespace smartCam
