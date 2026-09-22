#pragma once

#include "vp_entity.h"
#include "vp_result.h"

#include <QtGlobal>

class QDataStream;

namespace Vp
{

void writeEntityRecord(QDataStream& stream, const VpEntityRecord& entity);
VpResult<VpEntityRecord> readEntityRecord(QDataStream& stream, quint32 format_version);

} // namespace Vp
