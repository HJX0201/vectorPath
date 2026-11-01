#pragma once

#include "vp_entity.h"
#include "vp_result.h"

class QDataStream;

namespace Vp
{

// 把标注图元写入二进制流。
void writeDimensionData(QDataStream& stream, const VpLinearDimensionEntity& dimension);
// 从二进制流读出标注图元。
VpResult<VpLinearDimensionEntity> readDimensionData(QDataStream& stream, quint32 format_version);

} // namespace Vp
