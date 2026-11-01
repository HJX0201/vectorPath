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

// 矢量导入的填充方式、间距与缩放设置。
struct VpVectorImportSettings
{
    VpImportFillMode fill_mode = VpImportFillMode::None;
    double fill_spacing = 1.0;
    double fill_angle_degrees = 0.0;
    double scale = 1.0;
    bool preserve_outlines = true;
    bool include_color_blocks = true;
};

// 带颜色的图元几何，用于矢量导入。
struct VpColoredEntityGeometry
{
    VpEntityGeometry geometry = VpLineEntity{};
    VpEntityType type = VpEntityType::Line;
    QColor color{0, 0, 0};
};

// 矢量导入生成的图元集合与画布尺寸。
struct VpVectorImportGeometry
{
    std::vector<VpColoredEntityGeometry> entities;
    double width = 0.0;
    double height = 0.0;
    std::size_t region_count = 0;
    QStringList warnings;
};

// 用单线填充把区域转为多段线。
VpResult<std::vector<VpPolylineEntity>> createSingleLineFill(const VpVectorRegion& region,
                                                             double spacing, double angle_degrees);
// 用轮廓偏移生成填充多段线。
VpResult<std::vector<VpPolylineEntity>> createPolygonOffsetFill(const VpVectorRegion& region,
                                                                double spacing);
// 把区域转换为填充图元。
VpResult<std::vector<VpHatchEntity>> createRegionHatches(const VpVectorRegion& region);
// 把 SVG 矢量数据转换为可导入的图元几何。
VpResult<VpVectorImportGeometry> createVectorImportGeometry(const VpSvgVectorData& vector_data,
                                                            const VpVectorImportSettings& settings);

} // namespace Vp
