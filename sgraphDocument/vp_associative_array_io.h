#pragma once

#include "vp_entity.h"

#include <QDataStream>
#include <optional>

namespace Vp
{

void writeAssociativeArrayData(QDataStream& stream,
                               const std::optional<VpAssociativeArrayData>& array_data);
bool readAssociativeArrayData(QDataStream& stream,
                              std::optional<VpAssociativeArrayData>& array_data,
                              quint32 format_version);

} // namespace Vp
