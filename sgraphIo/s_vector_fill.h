#pragma once

#include "s_entity.h"
#include "s_result.h"
#include "s_svg_vector_data.h"

#include <QColor>
#include <QStringList>
#include <cstdint>
#include <vector>

namespace smartCam
{

enum class SImportFillMode : std::uint8_t
{
    None,
    SingleLine,
    PolygonOffset
};

struct SVectorImportSettings
{
    SImportFillMode fill_mode = SImportFillMode::None;
    double fill_spacing = 1.0;
    double fill_angle_degrees = 0.0;
    double scale = 1.0;
    bool preserve_outlines = true;
    bool include_color_blocks = true;
};

struct SColoredEntityGeometry
{
    SEntityGeometry geometry = SLineEntity{};
    SEntityType type = SEntityType::Line;
    QColor color{0, 0, 0};
};

struct SVectorImportGeometry
{
    std::vector<SColoredEntityGeometry> entities;
    double width = 0.0;
    double height = 0.0;
    std::size_t region_count = 0;
    QStringList warnings;
};

SResult<std::vector<SPolylineEntity>> createSingleLineFill(
    const SVectorRegion& region, double spacing, double angle_degrees);
SResult<std::vector<SPolylineEntity>> createPolygonOffsetFill(
    const SVectorRegion& region, double spacing);
SResult<std::vector<SHatchEntity>> createRegionHatches(const SVectorRegion& region);
SResult<SVectorImportGeometry> createVectorImportGeometry(
    const SSvgVectorData& vector_data, const SVectorImportSettings& settings);

} // namespace smartCam
