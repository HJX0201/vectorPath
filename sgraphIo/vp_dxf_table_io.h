#pragma once

#include "vp_dxf_pair.h"
#include "vp_layer_record.h"

#include <QString>
#include <vector>

class QTextStream;

namespace Vp
{

class VpCadDocument;

// 解析图层表记录。
std::vector<VpLayerRecord> readDxfLayerTable(const std::vector<VpDxfPair>& pairs);
// 解析当前图层名称。
QString readDxfCurrentLayer(const std::vector<VpDxfPair>& pairs);
// 把图层表应用到文档。
void applyDxfLayerTable(VpCadDocument& document, const std::vector<VpLayerRecord>& layers,
                        const QString& current_layer_name);

} // namespace Vp
