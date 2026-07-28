#pragma once

#include "s_result.h"

#include <QString>

namespace smartGraphics
{

class SCadDocument;

SResult<void> prepareLibreDwgR2000Dxf(const QString& source_path, const QString& target_path,
                                      const SCadDocument& document);

} // namespace smartGraphics
