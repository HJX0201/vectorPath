#pragma once

#include "vp_entity.h"
#include "vp_result.h"

class QDataStream;

namespace Vp
{

void writeHatchData(QDataStream& stream, const VpHatchEntity& hatch);
VpResult<VpHatchEntity> readHatchData(QDataStream& stream, quint32 format_version);

} // namespace Vp
