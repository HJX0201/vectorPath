#pragma once

#include "s_entity.h"

namespace smartCam
{

bool offsetCurveEntity(const SEntityRecord& source, const SPoint2d& through_point,
                       SEntityRecord& result);

} // namespace smartCam
