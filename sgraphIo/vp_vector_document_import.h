#pragma once

#include "vp_result.h"
#include "vp_vector_fill.h"

#include <cstddef>

namespace Vp
{

class VpCadDocument;

// 矢量图形导入文档的统计结果。
struct VpVectorDocumentImportReport
{
    std::size_t imported_entity_count = 0;
    std::size_t created_layer_count = 0;
};

// 把矢量几何导入文档并生成图元。
VpResult<VpVectorDocumentImportReport> importVectorGeometry(VpCadDocument& document,
                                                            const VpVectorImportGeometry& geometry);

} // namespace Vp
