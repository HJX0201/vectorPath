#include "vp_dxf_hatch_io.h"

#include "vp_hatch_geometry.h"
#include "vp_qt_text.h"

#include <QTextStream>
#include <cmath>
#include <iterator>
#include <optional>

namespace Vp
{
namespace
{

void writeEntityCommon(QTextStream& stream, const VpEntityRecord& entity)
{
    writeDxfPair(stream, 8, entity.layer_name);
    const int dxf_line_width = entity.line_width_mm < 0.0
                                   ? -1
                                   : static_cast<int>(std::round(entity.line_width_mm * 100.0));
    writeDxfPair(stream, 370, QString::number(dxf_line_width));
}

} // namespace

VpResult<VpHatchEntity> readDxfHatch(const std::vector<VpDxfPair>& entity_pairs)
{
    VpHatchEntity hatch;
    std::vector<std::vector<VpPoint2d>> loops;
    std::optional<double> pending_x;
    bool is_reading_loop = false;
    std::vector<QRgb> gradient_colors;
    for (const VpDxfPair& entity_pair : entity_pairs)
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
    hatch.pattern_name = findDxfString(entity_pairs, 2).value_or(QStringLiteral("ANSI31"));
    hatch.pattern_scale = findDxfDouble(entity_pairs, 41).value_or(1.0);
    hatch.pattern_angle = findDxfDouble(entity_pairs, 52).value_or(45.0);
    const bool is_gradient = findDxfDouble(entity_pairs, 450).value_or(0.0) > 0.5;
    const bool is_solid = findDxfDouble(entity_pairs, 70).value_or(0.0) > 0.5;
    hatch.fill_type = is_gradient ? VpHatchFillType::Gradient
                                  : (is_solid ? VpHatchFillType::Solid : VpHatchFillType::Pattern);
    if (!gradient_colors.empty())
    {
        hatch.gradient_start = QColor::fromRgb(gradient_colors.front());
        hatch.gradient_end = QColor::fromRgb(gradient_colors.back());
    }
    if (!isHatchValid(hatch))
    {
        return VpResult<VpHatchEntity>::failure(
            toCoreText(QStringLiteral("HATCH 边界或参数无效。")));
    }
    return VpResult<VpHatchEntity>::success(std::move(hatch));
}

void writeDxfHatch(QTextStream& stream, const VpEntityRecord& entity)
{
    const auto& hatch = std::get<VpHatchEntity>(entity.geometry);
    writeDxfPair(stream, 0, QStringLiteral("HATCH"));
    writeEntityCommon(stream, entity);
    writeDxfPair(stream, 100, QStringLiteral("AcDbHatch"));
    writeDxfPair(stream, 10, 0.0);
    writeDxfPair(stream, 20, 0.0);
    writeDxfPair(stream, 30, 0.0);
    writeDxfPair(stream, 2,
                 hatch.fill_type == VpHatchFillType::Solid ? QStringLiteral("SOLID")
                                                           : hatch.pattern_name);
    writeDxfPair(stream, 70,
                 hatch.fill_type == VpHatchFillType::Solid ? QStringLiteral("1")
                                                           : QStringLiteral("0"));
    writeDxfPair(stream, 71,
                 hatch.associative_boundary_id == 0 ? QStringLiteral("0") : QStringLiteral("1"));
    writeDxfPair(stream, 91, QString::number(hatch.island_boundaries.size() + 1));
    const auto write_loop = [&stream](const std::vector<VpPoint2d>& loop, bool is_outer)
    {
        writeDxfPair(stream, 92, is_outer ? QStringLiteral("3") : QStringLiteral("16"));
        writeDxfPair(stream, 93, QString::number(loop.size()));
        for (const VpPoint2d& point : loop)
        {
            writeDxfPair(stream, 10, point.x);
            writeDxfPair(stream, 20, point.y);
        }
        writeDxfPair(stream, 97, QStringLiteral("0"));
    };
    write_loop(hatch.boundary, true);
    for (const std::vector<VpPoint2d>& island : hatch.island_boundaries)
    {
        write_loop(island, false);
    }
    writeDxfPair(stream, 75, QStringLiteral("0"));
    writeDxfPair(stream, 76, QStringLiteral("1"));
    writeDxfPair(stream, 52, hatch.pattern_angle);
    writeDxfPair(stream, 41, hatch.pattern_scale);
    writeDxfPair(stream, 77, QStringLiteral("0"));
    if (hatch.fill_type == VpHatchFillType::Gradient)
    {
        const auto true_color = [](const QColor& color)
        {
            return (color.red() << 16) | (color.green() << 8) | color.blue();
        };
        writeDxfPair(stream, 450, QStringLiteral("1"));
        writeDxfPair(stream, 460, hatch.pattern_angle * 3.14159265358979323846 / 180.0);
        writeDxfPair(stream, 452, QStringLiteral("0"));
        writeDxfPair(stream, 453, QStringLiteral("2"));
        writeDxfPair(stream, 463, 0.0);
        writeDxfPair(stream, 421, QString::number(true_color(hatch.gradient_start)));
        writeDxfPair(stream, 463, 1.0);
        writeDxfPair(stream, 421, QString::number(true_color(hatch.gradient_end)));
        writeDxfPair(stream, 470, QStringLiteral("LINEAR"));
    }
}

} // namespace Vp
