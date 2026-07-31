#include "s_dxf_hatch_io.h"

#include "s_hatch_geometry.h"

#include <QTextStream>
#include <cmath>
#include <iterator>
#include <optional>

namespace smartCam
{
namespace
{

void writePair(QTextStream& stream, int group_code, const QString& value)
{
    stream << group_code << "\n" << value << "\n";
}

void writePair(QTextStream& stream, int group_code, double value)
{
    stream << group_code << "\n" << QString::number(value, 'g', 16) << "\n";
}

std::optional<double> findDouble(const std::vector<SDxfPair>& pairs, int group_code)
{
    for (const SDxfPair& pair : pairs)
    {
        if (pair.group_code == group_code)
        {
            bool is_valid = false;
            const double value = pair.value.toDouble(&is_valid);
            if (is_valid)
            {
                return value;
            }
        }
    }
    return std::nullopt;
}

std::optional<QString> findString(const std::vector<SDxfPair>& pairs, int group_code)
{
    for (const SDxfPair& pair : pairs)
    {
        if (pair.group_code == group_code)
        {
            return pair.value;
        }
    }
    return std::nullopt;
}

void writeEntityCommon(QTextStream& stream, const SEntityRecord& entity)
{
    writePair(stream, 8, entity.layer_name);
    const int dxf_line_width = entity.line_width_mm < 0.0
                                   ? -1
                                   : static_cast<int>(std::round(entity.line_width_mm * 100.0));
    writePair(stream, 370, QString::number(dxf_line_width));
}

} // namespace

SResult<SHatchEntity> readDxfHatch(const std::vector<SDxfPair>& entity_pairs)
{
    SHatchEntity hatch;
    std::vector<std::vector<SPoint2d>> loops;
    std::optional<double> pending_x;
    bool is_reading_loop = false;
    std::vector<QRgb> gradient_colors;
    for (const SDxfPair& entity_pair : entity_pairs)
    {
        if (entity_pair.group_code == 92)
        {
            loops.emplace_back();
            is_reading_loop = true;
            pending_x.reset();
        }
        else if (is_reading_loop && entity_pair.group_code == 10)
        {
            bool is_valid = false;
            const double value = entity_pair.value.toDouble(&is_valid);
            pending_x = is_valid ? std::optional<double>(value) : std::nullopt;
        }
        else if (is_reading_loop && entity_pair.group_code == 20 && pending_x)
        {
            bool is_valid = false;
            const double value = entity_pair.value.toDouble(&is_valid);
            if (is_valid)
            {
                loops.back().push_back({*pending_x, value});
            }
            pending_x.reset();
        }
        else if (entity_pair.group_code == 421)
        {
            bool is_valid = false;
            const uint color_value = entity_pair.value.toUInt(&is_valid);
            if (is_valid)
            {
                gradient_colors.push_back(qRgb((color_value >> 16) & 0xff,
                                               (color_value >> 8) & 0xff, color_value & 0xff));
            }
        }
    }
    if (!loops.empty())
    {
        hatch.boundary = std::move(loops.front());
        hatch.island_boundaries.assign(std::make_move_iterator(loops.begin() + 1),
                                       std::make_move_iterator(loops.end()));
    }
    hatch.pattern_name = findString(entity_pairs, 2).value_or(QStringLiteral("ANSI31"));
    hatch.pattern_scale = findDouble(entity_pairs, 41).value_or(1.0);
    hatch.pattern_angle = findDouble(entity_pairs, 52).value_or(45.0);
    const bool is_gradient = findDouble(entity_pairs, 450).value_or(0.0) > 0.5;
    const bool is_solid = findDouble(entity_pairs, 70).value_or(0.0) > 0.5;
    hatch.fill_type = is_gradient ? SHatchFillType::Gradient
                                  : (is_solid ? SHatchFillType::Solid : SHatchFillType::Pattern);
    if (!gradient_colors.empty())
    {
        hatch.gradient_start = QColor::fromRgb(gradient_colors.front());
        hatch.gradient_end = QColor::fromRgb(gradient_colors.back());
    }
    if (!isHatchValid(hatch))
    {
        return SResult<SHatchEntity>::failure(QStringLiteral("HATCH 边界或参数无效。"));
    }
    return SResult<SHatchEntity>::success(std::move(hatch));
}

void writeDxfHatch(QTextStream& stream, const SEntityRecord& entity)
{
    const auto& hatch = std::get<SHatchEntity>(entity.geometry);
    writePair(stream, 0, QStringLiteral("HATCH"));
    writeEntityCommon(stream, entity);
    writePair(stream, 100, QStringLiteral("AcDbHatch"));
    writePair(stream, 10, 0.0);
    writePair(stream, 20, 0.0);
    writePair(stream, 30, 0.0);
    writePair(stream, 2,
              hatch.fill_type == SHatchFillType::Solid ? QStringLiteral("SOLID")
                                                       : hatch.pattern_name);
    writePair(stream, 70,
              hatch.fill_type == SHatchFillType::Solid ? QStringLiteral("1") : QStringLiteral("0"));
    writePair(stream, 71,
              hatch.associative_boundary_id == 0 ? QStringLiteral("0") : QStringLiteral("1"));
    writePair(stream, 91, QString::number(hatch.island_boundaries.size() + 1));
    const auto write_loop = [&stream](const std::vector<SPoint2d>& loop, bool is_outer)
    {
        writePair(stream, 92, is_outer ? QStringLiteral("3") : QStringLiteral("16"));
        writePair(stream, 93, QString::number(loop.size()));
        for (const SPoint2d& point : loop)
        {
            writePair(stream, 10, point.x);
            writePair(stream, 20, point.y);
        }
        writePair(stream, 97, QStringLiteral("0"));
    };
    write_loop(hatch.boundary, true);
    for (const std::vector<SPoint2d>& island : hatch.island_boundaries)
    {
        write_loop(island, false);
    }
    writePair(stream, 75, QStringLiteral("0"));
    writePair(stream, 76, QStringLiteral("1"));
    writePair(stream, 52, hatch.pattern_angle);
    writePair(stream, 41, hatch.pattern_scale);
    writePair(stream, 77, QStringLiteral("0"));
    if (hatch.fill_type == SHatchFillType::Gradient)
    {
        const auto true_color = [](const QColor& color)
        {
            return (color.red() << 16) | (color.green() << 8) | color.blue();
        };
        writePair(stream, 450, QStringLiteral("1"));
        writePair(stream, 460, hatch.pattern_angle * 3.14159265358979323846 / 180.0);
        writePair(stream, 452, QStringLiteral("0"));
        writePair(stream, 453, QStringLiteral("2"));
        writePair(stream, 463, 0.0);
        writePair(stream, 421, QString::number(true_color(hatch.gradient_start)));
        writePair(stream, 463, 1.0);
        writePair(stream, 421, QString::number(true_color(hatch.gradient_end)));
        writePair(stream, 470, QStringLiteral("LINEAR"));
    }
}

} // namespace smartCam
