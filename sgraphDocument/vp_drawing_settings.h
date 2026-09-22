#pragma once

#include <QString>
#include <optional>

namespace Vp
{

enum class VpInsertionUnit
{
    Unitless,
    Millimeters,
    Centimeters,
    Meters,
    Inches,
    Feet
};

enum class VpAngleFormat
{
    DecimalDegrees,
    DegreesMinutesSeconds,
    Gradians,
    Radians
};

struct VpDrawingSettings
{
    VpInsertionUnit insertion_unit = VpInsertionUnit::Millimeters;
    VpAngleFormat angle_format = VpAngleFormat::DecimalDegrees;
    int linear_precision = 3;
    int angular_precision = 2;
};

bool operator==(const VpDrawingSettings& left, const VpDrawingSettings& right) noexcept;
bool operator!=(const VpDrawingSettings& left, const VpDrawingSettings& right) noexcept;
bool isValidDrawingSettings(const VpDrawingSettings& settings) noexcept;
QString insertionUnitKey(VpInsertionUnit unit);
QString insertionUnitSymbol(VpInsertionUnit unit);
std::optional<VpInsertionUnit> insertionUnitFromKey(const QString& key);
QString angleFormatKey(VpAngleFormat format);
std::optional<VpAngleFormat> angleFormatFromKey(const QString& key);
QString formatLinearValue(double value, const VpDrawingSettings& settings);
QString formatAngleValue(double angle_degrees, const VpDrawingSettings& settings);

} // namespace Vp
