#pragma once

#include "vp_entity.h"
#include "vp_result.h"

class QDataStream;

namespace Vp
{

// 把填充数据写入二进制流。
void writeHatchData(QDataStream& stream, const VpHatchEntity& hatch);
// 从二进制流读出填充数据。
VpResult<VpHatchEntity> readHatchData(QDataStream& stream, quint32 format_version);

} // namespace Vp
