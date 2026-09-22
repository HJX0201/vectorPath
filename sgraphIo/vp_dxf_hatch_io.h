#pragma once

#include "vp_dxf_pair.h"
#include "vp_entity.h"
#include "vp_result.h"

#include <QString>
#include <vector>

class QTextStream;

namespace Vp
{

VpResult<VpHatchEntity> readDxfHatch(const std::vector<VpDxfPair>& entity_pairs);
void writeDxfHatch(QTextStream& stream, const VpEntityRecord& entity);

} // namespace Vp
