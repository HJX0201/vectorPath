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

struct VpCoordinateInput
{
    VpCoordinateInputMode mode = VpCoordinateInputMode::NotCoordinate;
    double first_value = 0.0;
    double second_value = 0.0;
};

VpCoordinateInput parseCoordinateInput(const QString& text);
std::optional<VpPoint2d> resolveCoordinateInput(const VpCoordinateInput& input,
                                                const std::optional<VpPoint2d>& reference_point);

} // namespace Vp
