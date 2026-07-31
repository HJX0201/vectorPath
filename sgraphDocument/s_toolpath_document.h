#pragma once

#include "s_toolpath.h"

#include <vector>

namespace smartCam
{

class SCadDocument;

struct SToolpathDocumentSortResult
{
    int sorted_entity_count = 0;
    int reversed_entity_count = 0;
};

SToolpathDocumentSortResult sortDocumentToolpaths(
    SCadDocument& document, const std::vector<SEntityId>& selected_entity_ids,
    const SToolpathSortOptions& options);

} // namespace smartCam
