#pragma once

#include "s_result.h"
#include "s_svg_vector_data.h"

#include <QByteArray>

namespace smartCam
{

SResult<SSvgVectorData> parseSvgVectorData(const QByteArray& svg_data);

} // namespace smartCam
