#pragma once

#include "s_entity.h"

namespace smartGraphics
{

bool offsetCurveEntity(const SEntityRecord& source, const SPoint2d& through_point,
                       SEntityRecord& result);

} // namespace smartGraphics
