#pragma once

#include "vp_entity.h"
#include "vp_result.h"

class QDataStream;

namespace Vp
{

void writeDimensionData(QDataStream& stream, const VpLinearDimensionEntity& dimension);
VpResult<VpLinearDimensionEntity> readDimensionData(QDataStream& stream, quint32 format_version);

} // namespace Vp
