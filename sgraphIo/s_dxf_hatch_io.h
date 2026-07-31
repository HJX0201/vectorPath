#pragma once

#include "s_dxf_pair.h"
#include "s_entity.h"
#include "s_result.h"

#include <QString>
#include <vector>

class QTextStream;

namespace smartCam
{

SResult<SHatchEntity> readDxfHatch(const std::vector<SDxfPair>& entity_pairs);
void writeDxfHatch(QTextStream& stream, const SEntityRecord& entity);

} // namespace smartCam
