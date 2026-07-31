#pragma once

#include "s_entity.h"
#include "s_result.h"

class QDataStream;

namespace smartCam
{

void writeHatchData(QDataStream& stream, const SHatchEntity& hatch);
SResult<SHatchEntity> readHatchData(QDataStream& stream, quint32 format_version);

} // namespace smartCam
