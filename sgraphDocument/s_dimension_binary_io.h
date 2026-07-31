#pragma once

#include "s_entity.h"
#include "s_result.h"

class QDataStream;

namespace smartCam
{

void writeDimensionData(QDataStream& stream, const SLinearDimensionEntity& dimension);
SResult<SLinearDimensionEntity> readDimensionData(QDataStream& stream, quint32 format_version);

} // namespace smartCam
