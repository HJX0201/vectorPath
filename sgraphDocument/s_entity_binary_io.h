#pragma once

#include "s_entity.h"
#include "s_result.h"

#include <QtGlobal>

class QDataStream;

namespace vectorPath
{

void writeEntityRecord(QDataStream& stream, const SEntityRecord& entity);
SResult<SEntityRecord> readEntityRecord(QDataStream& stream, quint32 format_version);

} // namespace vectorPath
