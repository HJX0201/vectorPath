#pragma once

#include "vp_dxf_pair.h"
#include "vp_entity.h"
#include "vp_file_compatibility_report.h"
#include "vp_result.h"

#include <QString>
#include <vector>

class QTextStream;

namespace Vp
{

VpResult<VpEntityRecord> readDxfEntity(const QString& entity_name,
                                       const std::vector<VpDxfPair>& entity_pairs);
bool writeDxfEntity(QTextStream& stream, const VpEntityRecord& entity,
                    VpFileCompatibilityReport& report, bool count_entity = true);

} // namespace Vp
