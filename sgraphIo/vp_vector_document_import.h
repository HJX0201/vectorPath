#pragma once

#include "vp_result.h"
#include "vp_vector_fill.h"

#include <cstddef>

namespace Vp
{

class VpCadDocument;

struct VpVectorDocumentImportReport
{
    std::size_t imported_entity_count = 0;
    std::size_t created_layer_count = 0;
};

VpResult<VpVectorDocumentImportReport> importVectorGeometry(VpCadDocument& document,
                                                            const VpVectorImportGeometry& geometry);

} // namespace Vp
