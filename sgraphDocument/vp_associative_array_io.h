#pragma once

#include "vp_entity.h"

#include <QDataStream>
#include <optional>

namespace Vp
{

// 把关联阵列参数写入二进制流。
void writeAssociativeArrayData(QDataStream& stream,
                               const std::optional<VpAssociativeArrayData>& array_data);
// 从二进制流读出关联阵列参数。
bool readAssociativeArrayData(QDataStream& stream,
                              std::optional<VpAssociativeArrayData>& array_data,
                              quint32 format_version);

} // namespace Vp
