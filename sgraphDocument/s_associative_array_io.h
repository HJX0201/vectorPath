#pragma once

#include "s_entity.h"

#include <QDataStream>
#include <optional>

namespace smartCam
{

void writeAssociativeArrayData(QDataStream& stream,
                               const std::optional<SAssociativeArrayData>& array_data);
bool readAssociativeArrayData(QDataStream& stream, std::optional<SAssociativeArrayData>& array_data,
                              quint32 format_version);

} // namespace smartCam
