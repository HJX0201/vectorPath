#pragma once

#include "vp_entity.h"
#include "vp_result.h"
#include "vp_svg_vector_data.h"
#include "vp_vector_fill.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Vp
{

class VpCadDocument;

enum class VpSvgLayerPriority : std::uint8_t
{
    UpperFirst,
    LowerFirst
};

struct VpSvgFillReport
{
    std::size_t source_region_count = 0;
    std::size_t created_polyline_count = 0;
    std::size_t replaced_entity_count = 0;
};

struct VpSvgDeduplicateReport
{
    std::size_t processed_layer_count = 0;
    std::size_t source_region_count = 0;
    std::size_t result_region_count = 0;
};

VpSvgVectorData svgColorBlockVectorData(const VpCadDocument& document,
                                        const std::vector<VpEntityId>& target_entity_ids = {});
VpResult<VpSvgFillReport> fillSvgColorBlocks(VpCadDocument& document,
                                             const VpVectorImportSettings& settings,
                                             const std::vector<VpEntityId>& target_entity_ids = {});
VpResult<VpSvgDeduplicateReport> deduplicateSvgColorBlocks(VpCadDocument& document,
                                                           VpSvgLayerPriority priority);

} // namespace Vp
