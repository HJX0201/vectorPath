#pragma once

#include "vp_toolpath.h"

#include <vector>

namespace Vp
{

class VpCadDocument;

struct VpToolpathDocumentSortResult
{
    int sorted_entity_count = 0;
    int reversed_entity_count = 0;
};

VpToolpathDocumentSortResult
sortDocumentToolpaths(VpCadDocument& document, const std::vector<VpEntityId>& selected_entity_ids,
                      const VpToolpathSortOptions& options);

} // namespace Vp
