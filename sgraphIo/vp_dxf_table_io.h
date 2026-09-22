#pragma once

#include "vp_dxf_pair.h"
#include "vp_layer_record.h"

#include <QString>
#include <vector>

class QTextStream;

namespace Vp
{

class VpCadDocument;

std::vector<VpLayerRecord> readDxfLayerTable(const std::vector<VpDxfPair>& pairs);
QString readDxfCurrentLayer(const std::vector<VpDxfPair>& pairs);
void applyDxfLayerTable(VpCadDocument& document, const std::vector<VpLayerRecord>& layers,
                        const QString& current_layer_name);

} // namespace Vp
