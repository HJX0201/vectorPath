#pragma once

#include "s_result.h"
#include "s_vector_fill.h"

#include <cstddef>

namespace smartGraphics
{

class SCadDocument;

struct SVectorDocumentImportReport
{
    std::size_t imported_entity_count = 0;
    std::size_t created_layer_count = 0;
};

SResult<SVectorDocumentImportReport> importVectorGeometry(
    SCadDocument& document, const SVectorImportGeometry& geometry);

} // namespace smartGraphics
