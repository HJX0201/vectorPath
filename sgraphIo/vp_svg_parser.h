#pragma once

#include "vp_result.h"
#include "vp_svg_vector_data.h"

#include <QByteArray>

namespace Vp
{

VpResult<VpSvgVectorData> parseSvgVectorData(const QByteArray& svg_data);

} // namespace Vp
