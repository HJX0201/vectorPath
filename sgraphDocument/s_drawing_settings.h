#pragma once

#include <QString>
#include <optional>

namespace smartGraphics
{

enum class SInsertionUnit
{
    Unitless,
    Millimeters,
    Centimeters,
    Meters,
    Inches,
    Feet
};

enum class SAngleFormat
{
    DecimalDegrees,
    DegreesMinutesSeconds,
    Gradians,
    Radians
};

struct SDrawingSettings
{
    SInsertionUnit insertion_unit = SInsertionUnit::Millimeters;
    SAngleFormat angle_format = SAngleFormat::DecimalDegrees;
    int linear_precision = 3;
    int angular_precision = 2;
};

bool operator==(const SDrawingSettings& left, const SDrawingSettings& right) noexcept;
bool operator!=(const SDrawingSettings& left, const SDrawingSettings& right) noexcept;
bool isValidDrawingSettings(const SDrawingSettings& settings) noexcept;
QString insertionUnitKey(SInsertionUnit unit);
QString insertionUnitSymbol(SInsertionUnit unit);
std::optional<SInsertionUnit> insertionUnitFromKey(const QString& key);
QString angleFormatKey(SAngleFormat format);
std::optional<SAngleFormat> angleFormatFromKey(const QString& key);
QString formatLinearValue(double value, const SDrawingSettings& settings);
QString formatAngleValue(double angle_degrees, const SDrawingSettings& settings);

} // namespace smartGraphics
