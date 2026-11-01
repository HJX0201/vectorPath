#pragma once

#include "vp_geometry_types.h"

#include <QString>
#include <optional>

namespace Vp
{

enum class VpCoordinateInputMode
{
    NotCoordinate,
    Invalid,
    AbsoluteCartesian,
    RelativeCartesian,
    RelativePolar
};

// 命令行坐标输入（绝对/相对/极坐标）的解析结果。
struct VpCoordinateInput
{
    VpCoordinateInputMode mode = VpCoordinateInputMode::NotCoordinate;
    double first_value = 0.0;
    double second_value = 0.0;
};

// 解析命令行文本为坐标输入。
VpCoordinateInput parseCoordinateInput(const QString& text);
// 结合参考点把坐标输入解析为绝对坐标。
std::optional<VpPoint2d> resolveCoordinateInput(const VpCoordinateInput& input,
                                                const std::optional<VpPoint2d>& reference_point);

} // namespace Vp
