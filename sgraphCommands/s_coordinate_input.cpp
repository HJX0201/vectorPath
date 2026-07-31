#include "s_coordinate_input.h"

#include <QStringList>
#include <cmath>

namespace vectorPath
{
namespace
{

constexpr double kPi = 3.14159265358979323846;

bool parsePair(const QString& text, QChar separator, double& first_value, double& second_value)
{
    const QStringList parts = text.split(separator);
    if (parts.size() != 2)
    {
        return false;
    }

    bool is_first_valid = false;
    bool is_second_valid = false;
    first_value = parts[0].trimmed().toDouble(&is_first_valid);
    second_value = parts[1].trimmed().toDouble(&is_second_valid);
    return is_first_valid && is_second_valid && std::isfinite(first_value) &&
           std::isfinite(second_value);
}

} // namespace

SCoordinateInput parseCoordinateInput(const QString& text)
{
    const QString trimmed_text = text.trimmed();
    if (trimmed_text.isEmpty())
    {
        return {};
    }

    const bool is_relative = trimmed_text.startsWith(QLatin1Char('@'));
    const QString coordinate_text = is_relative ? trimmed_text.mid(1).trimmed() : trimmed_text;
    const bool is_polar = coordinate_text.contains(QLatin1Char('<'));
    const bool is_cartesian = coordinate_text.contains(QLatin1Char(','));
    if (!is_relative && !is_cartesian)
    {
        return {};
    }

    SCoordinateInput input;
    if ((is_polar && is_cartesian) || (!is_polar && !is_cartesian))
    {
        input.mode = SCoordinateInputMode::Invalid;
        return input;
    }

    const QChar separator = is_polar ? QLatin1Char('<') : QLatin1Char(',');
    if (!parsePair(coordinate_text, separator, input.first_value, input.second_value))
    {
        input.mode = SCoordinateInputMode::Invalid;
        return input;
    }

    if (is_polar)
    {
        input.mode =
            is_relative ? SCoordinateInputMode::RelativePolar : SCoordinateInputMode::Invalid;
    }
    else
    {
        input.mode = is_relative ? SCoordinateInputMode::RelativeCartesian
                                 : SCoordinateInputMode::AbsoluteCartesian;
    }
    return input;
}

std::optional<SPoint2d> resolveCoordinateInput(const SCoordinateInput& input,
                                               const std::optional<SPoint2d>& reference_point)
{
    if (input.mode == SCoordinateInputMode::AbsoluteCartesian)
    {
        return SPoint2d{input.first_value, input.second_value};
    }
    if (!reference_point || (input.mode != SCoordinateInputMode::RelativeCartesian &&
                             input.mode != SCoordinateInputMode::RelativePolar))
    {
        return std::nullopt;
    }
    if (input.mode == SCoordinateInputMode::RelativeCartesian)
    {
        return SPoint2d{reference_point->x + input.first_value,
                        reference_point->y + input.second_value};
    }

    const double angle_radians = input.second_value * kPi / 180.0;
    return SPoint2d{reference_point->x + (input.first_value * std::cos(angle_radians)),
                    reference_point->y + (input.first_value * std::sin(angle_radians))};
}

} // namespace vectorPath
