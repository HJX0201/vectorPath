#pragma once

#include "s_dxf_pair.h"
#include "s_entity.h"
#include "s_file_compatibility_report.h"
#include "s_result.h"

#include <QString>
#include <vector>

class QTextStream;

namespace smartGraphics
{

SResult<SEntityRecord> readDxfEntity(const QString& entity_name,
                                     const std::vector<SDxfPair>& entity_pairs);
bool writeDxfEntity(QTextStream& stream, const SEntityRecord& entity,
                    SFileCompatibilityReport& report, bool count_entity = true);

} // namespace smartGraphics
