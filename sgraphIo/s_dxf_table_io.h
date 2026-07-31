#pragma once

#include "s_dxf_pair.h"
#include "s_layer_record.h"

#include <QString>
#include <vector>

class QTextStream;

namespace vectorPath
{

class SCadDocument;

std::vector<SLayerRecord> readDxfLayerTable(const std::vector<SDxfPair>& pairs);
QString readDxfCurrentLayer(const std::vector<SDxfPair>& pairs);
void applyDxfLayerTable(SCadDocument& document, const std::vector<SLayerRecord>& layers,
                        const QString& current_layer_name);

} // namespace vectorPath
