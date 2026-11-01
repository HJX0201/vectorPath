#pragma once

#include "vp_result.h"

#include <QString>

namespace Vp
{

class VpCadDocument;

// 用 libredwg 把 DWG 转成 R2000 DXF 供解析。
VpResult<void> prepareLibreDwgR2000Dxf(const QString& source_path, const QString& target_path,
                                       const VpCadDocument& document);

} // namespace Vp
