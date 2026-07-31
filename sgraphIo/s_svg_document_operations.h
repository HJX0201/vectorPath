#pragma once

#include "s_entity.h"
#include "s_result.h"
#include "s_svg_vector_data.h"
#include "s_vector_fill.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace smartCam
{

class SCadDocument;

enum class SSvgLayerPriority : std::uint8_t
{
    UpperFirst,
    LowerFirst
};

struct SSvgFillReport
{
    std::size_t source_region_count = 0;
    std::size_t created_polyline_count = 0;
    std::size_t replaced_entity_count = 0;
};

struct SSvgDeduplicateReport
{
    std::size_t processed_layer_count = 0;
    std::size_t source_region_count = 0;
    std::size_t result_region_count = 0;
};

SSvgVectorData svgColorBlockVectorData(
    const SCadDocument& document,
    const std::vector<SEntityId>& target_entity_ids = {});
SResult<SSvgFillReport> fillSvgColorBlocks(
    SCadDocument& document, const SVectorImportSettings& settings,
    const std::vector<SEntityId>& target_entity_ids = {});
SResult<SSvgDeduplicateReport> deduplicateSvgColorBlocks(
    SCadDocument& document, SSvgLayerPriority priority);

} // namespace smartCam
