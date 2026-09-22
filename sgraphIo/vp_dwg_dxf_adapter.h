#pragma once

#include "vp_result.h"

#include <QString>

namespace Vp
{

class VpCadDocument;

VpResult<void> prepareLibreDwgR2000Dxf(const QString& source_path, const QString& target_path,
                                       const VpCadDocument& document);

} // namespace Vp
