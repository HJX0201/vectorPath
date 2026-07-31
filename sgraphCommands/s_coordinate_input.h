#pragma once

#include "s_geometry_types.h"

#include <QString>
#include <optional>

namespace vectorPath
{

enum class SCoordinateInputMode
{
    NotCoordinate,
    Invalid,
    AbsoluteCartesian,
    RelativeCartesian,
    RelativePolar
};

struct SCoordinateInput
{
    SCoordinateInputMode mode = SCoordinateInputMode::NotCoordinate;
    double first_value = 0.0;
    double second_value = 0.0;
};

SCoordinateInput parseCoordinateInput(const QString& text);
std::optional<SPoint2d> resolveCoordinateInput(const SCoordinateInput& input,
                                               const std::optional<SPoint2d>& reference_point);

} // namespace vectorPath
