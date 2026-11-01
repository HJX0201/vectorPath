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

// 图纸级设置：插入单位、角度格式与精度。
struct VpDrawingSettings
{
    VpInsertionUnit insertion_unit = VpInsertionUnit::Millimeters;
    VpAngleFormat angle_format = VpAngleFormat::DecimalDegrees;
    int linear_precision = 3;
    int angular_precision = 2;
};

bool operator==(const VpDrawingSettings& left, const VpDrawingSettings& right) noexcept;
bool operator!=(const VpDrawingSettings& left, const VpDrawingSettings& right) noexcept;
// 判断图纸设置是否合法。
bool isValidDrawingSettings(const VpDrawingSettings& settings) noexcept;
// 返回插入单位的配置键。
QString insertionUnitKey(VpInsertionUnit unit);
// 返回插入单位的符号。
QString insertionUnitSymbol(VpInsertionUnit unit);
// 由配置键还原插入单位。
std::optional<VpInsertionUnit> insertionUnitFromKey(const QString& key);
// 返回角度格式的配置键。
QString angleFormatKey(VpAngleFormat format);
// 由配置键还原角度格式。
std::optional<VpAngleFormat> angleFormatFromKey(const QString& key);
// 按设置精度格式化线性值。
QString formatLinearValue(double value, const VpDrawingSettings& settings);
// 按设置精度格式化角度值。
QString formatAngleValue(double angle_degrees, const VpDrawingSettings& settings);

} // namespace Vp
