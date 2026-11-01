#pragma once

#include "vp_entity.h"
#include "vp_result.h"

#include <QtGlobal>

class QDataStream;

namespace Vp
{

// 把图元记录写入二进制流。
void writeEntityRecord(QDataStream& stream, const VpEntityRecord& entity);
// 从二进制流读出图元记录。
VpResult<VpEntityRecord> readEntityRecord(QDataStream& stream, quint32 format_version);

} // namespace Vp
