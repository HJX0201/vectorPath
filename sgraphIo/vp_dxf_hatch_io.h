#pragma once

#include "vp_dxf_pair.h"
#include "vp_entity.h"
#include "vp_result.h"

#include <QString>
#include <vector>

class QTextStream;

namespace Vp
{

// 从 DXF 组码解析填充图元。
VpResult<VpHatchEntity> readDxfHatch(const std::vector<VpDxfPair>& entity_pairs);
// 把填充图元写成 DXF 组码。
void writeDxfHatch(QTextStream& stream, const VpEntityRecord& entity);

} // namespace Vp
