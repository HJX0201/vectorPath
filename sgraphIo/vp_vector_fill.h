#pragma once

#include "vp_entity.h"
#include "vp_result.h"
#include "vp_svg_vector_data.h"

#include <QColor>
#include <QStringList>
#include <cstdint>
#include <vector>

namespace Vp
{

enum class VpImportFillMode : std::uint8_t
{
    None,
    SingleLine,
    PolygonOffset
};

struct VpVectorImportSettings
{
    VpImportFillMode fill_mode = VpImportFillMode::None;
    double fill_spacing = 1.0;
    double fill_angle_degrees = 0.0;
    double scale = 1.0;
    bool preserve_outlines = true;
    bool include_color_blocks = true;
};

struct VpColoredEntityGeometry
{
    VpEntityGeometry geometry = VpLineEntity{};
    VpEntityType type = VpEntityType::Line;
    QColor color{0, 0, 0};
};

struct VpVectorImportGeometry
{
    std::vector<VpColoredEntityGeometry> entities;
    double width = 0.0;
    double height = 0.0;
    std::size_t region_count = 0;
    QStringList warnings;
};

VpResult<std::vector<VpPolylineEntity>> createSingleLineFill(const VpVectorRegion& region,
                                                             double spacing, double angle_degrees);
VpResult<std::vector<VpPolylineEntity>> createPolygonOffsetFill(const VpVectorRegion& region,
                                                                double spacing);
VpResult<std::vector<VpHatchEntity>> createRegionHatches(const VpVectorRegion& region);
VpResult<VpVectorImportGeometry> createVectorImportGeometry(const VpSvgVectorData& vector_data,
                                                            const VpVectorImportSettings& settings);

} // namespace Vp
